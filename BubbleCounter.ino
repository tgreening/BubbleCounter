#include <Ticker.h>
#include <ESP8266WiFi.h>
#include "ThingSpeak.h"
#include <WiFiManager.h>         //https://github.com/tzapu/WiFiManager
#include <ArduinoOTA.h>
#include <ESP8266mDNS.h>

// replace with your channel’s thingspeak API key and your SSID and password
const char * apiKey = "xxxxx";
const char* server = "api.thingspeak.com";
unsigned long myChannelNumber = 419446;
const char * host = "BubbleCounter";
WiFiManager wifiManager;

#define SENSORPIN D3

// variables will change:
long POST_MILLIS = 1800000UL;
long unsigned lastPostTime = ((long)millis()) + POST_MILLIS;
int sensorState = 0, lastState = 0;       // variable for reading the pushbutton status
WiFiClient client;

Ticker ticker;
const int UPDATE_INTERVAL_SECS = 60; // 10 minutes update intervall. Wunderground quota...

int bubbleCount = 0;

void setup()
{
  // initialize the LED pin as an output:
  pinMode(BUILTIN_LED, OUTPUT);
  // initialize the sensor pin as an input:
  pinMode(SENSORPIN, INPUT);
  digitalWrite(SENSORPIN, HIGH); // turn on the pullup

  Serial.begin(115200);

  WiFi.hostname(String(host));

  if (!wifiManager.autoConnect(host)) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    //reset and try again, or maybe put it to deep sleep
    ESP.reset();
    delay(5000);
  }

  //if you get here you have connected to the WiFi
  Serial.println("connected...yeey :)");
  if (!MDNS.begin(host)) {
    Serial.println("Error setting up MDNS responder!");
  }

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  WiFi.mode(WIFI_STA);

  ThingSpeak.begin(client);  // Initialize ThingSpeak
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH)
      type = "sketch";
    else // U_SPIFFS
      type = "filesystem";

    // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}

void loop()
{
  // read the state of the pushbutton value:
  sensorState = digitalRead(SENSORPIN);

  // check if the sensor beam is broken
  // if it is, the sensorState is LOW:
  if (sensorState == LOW) {
    // turn LED on:
    digitalWrite(BUILTIN_LED, HIGH);
  }
  else {
    // turn LED off:
    digitalWrite(BUILTIN_LED, LOW);
  }
  if (sensorState == HIGH && lastState != HIGH) {
    bubbleCount++;
  }
  if ((long)(millis() - lastPostTime) >= 0) {
    Serial.println("Posting....");
    lastPostTime += POST_MILLIS;
    updateThingSpeak(bubbleCount);
    bubbleCount = 0;
    yield();
  }
  lastState = sensorState;
  ArduinoOTA.handle();
}

void updateThingSpeak(int updateCount) {
  Serial.println(updateCount);
  Serial.println("About to post!");
  int x = ThingSpeak.writeField(myChannelNumber, 1, updateCount, apiKey);
  if (x == 200) {
    Serial.println("Channel update successful.");
  }
  else {
    Serial.println("Problem updating channel. HTTP error code " + String(x));
  }
  Serial.println(x);
}
