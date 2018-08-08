//include <FS.h>
#include "SPIFFS.h"
#include <WiFi.h>
#include <DNSServer.h>
//#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

/*
bool shouldSaveConfig = true;


void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
}
*/

void setup() {
  
  Serial.begin(115200);
  Serial.println();
  WiFiManager wifiManager;
  //wifiManager.resetSettings();
  Serial.print("SSID:");
  Serial.println(wifiManager.getSSID());
  Serial.print("Password:");
  Serial.println(wifiManager.getPassword());
  if (!wifiManager.autoConnect("Mushroom Device", "admin1234")) {
    Serial.println("failed to connect and hit timeout");
    delay(3000);
    ESP.restart();
    delay(5000);
  }
  Serial.println("connected");
  Serial.print("SSID:");
  Serial.println(wifiManager.getSSID());
  Serial.print("Password:");
  Serial.println(wifiManager.getPassword());
  
  }


void loop() {}
