#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial.println();//new line to clear noise/garbage

  //WiFiManager
  WiFiManager wifiManager;
  Serial.println("** resetting wifimanager settings. Pay no attention to the next 3 lines (settings invalidated, THIS MAY..., and new line) from wifimanager:**");
  wifiManager.resetSettings();  //reset settings - for testing
  //do we want to do something with ??: 
  //clean FS, for testing
  SPIFFS.format();
  Serial.println("Settings cleared.");
}

void loop() {

}
