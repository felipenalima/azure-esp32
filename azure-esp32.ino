#include <WiFi.h>
#include <QMC5883LCompass.h>
#include "AzureIotHub.h"
#include "Esp32MQTTClient.h"

#define INTERVAL 3000  //time interval between two consecutive messages in milliseconds
#define DEVICE_ID "ESP32Device"   //device Id you wish to give To identify the device

#define MESSAGE_MAX_LEN 256    //max message length 

QMC5883LCompass compass; //Magnometer Sensor

// Please input the SSID and password of WiFi
const char* ssid     = "YOUR WIFI NAME"; // Name of your Wi-Fi Network.
const char* password = "YOUR WIFI PASSWORD"; // Enter your Wi-Fi Network Security Key / Password

//Connection String
static const char* connectionString = "HostName=YourHostName.azure-devices.net;DeviceId=YourDevice;SharedAccessKey=YourSharedAcessKey"; 

int messageCount = 1;    //initialize message id 

//message format
const char *messageData = "{\"deviceId\":\"%s\", \"messageId\":%d, \"azimuth\":%d}"; 

static bool hasWifi = false;
static bool messageSending = true;
static uint64_t send_interval_ms;

//Initi Wifi
static void InitWifi()
{
  Serial.println("Connecting...");

  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {      
    delay(500);
    Serial.print(".");
  }
  
  hasWifi = true;
  
  Serial.println("WiFi connected");
  Serial.println("IP address: "); 
  Serial.println(WiFi.localIP());

}

//Function definition

static void SendConfirmationCallback(IOTHUB_CLIENT_CONFIRMATION_RESULT result)
{
  if (result == IOTHUB_CLIENT_CONFIRMATION_OK)
  {
    Serial.println("Send Confirmation Callback finished.");
  }
}

void setup()
{
  
  Serial.begin(9600);
  
  Serial.println(F("Magnometer Test"));
  Serial.println("Initializing...");

  //Init Sensor
  compass.init();

  //Initialize the WiFi module
  Serial.println(" > WiFi");
  hasWifi = false;
  
  InitWifi();
  
  if (!hasWifi)
  {
    return;
  }
  
  Serial.println(" > IoT Hub");
    
  //MQTT Functions  
  Esp32MQTTClient_Init((const uint8_t*)connectionString);
  Esp32MQTTClient_SetSendConfirmationCallback(SendConfirmationCallback);

  send_interval_ms = millis();
}

void loop()
{
  if (hasWifi)
  {
    if (messageSending && 
        (int)(millis() - send_interval_ms) >= INTERVAL)
    {
      char messagePayload[MESSAGE_MAX_LEN];
      compass.read();

      int azimuth = compass.getAzimuth();;

      snprintf(messagePayload,MESSAGE_MAX_LEN,messageData, DEVICE_ID, messageCount++, azimuth);
       
      Serial.println(messagePayload);
      
      //Esp32MQTTClient_Event_Generate(const char *eventString, EVENT_TYPE type);
      EVENT_INSTANCE* message = Esp32MQTTClient_Event_Generate(messagePayload, MESSAGE);
      
      Esp32MQTTClient_SendEventInstance(message);

      send_interval_ms = millis();
    }
  }
  delay(500);
}