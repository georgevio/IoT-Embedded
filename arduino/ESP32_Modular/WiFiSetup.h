#ifndef WIFISETUP_H
#define WIFISETUP_H

#include <WiFi.h>
#include "Config.h"

const char* ssid = "Casa Violetta";
const char* password = "$$123789$$";

void connectToWiFi() {
  WiFi.begin(ssid, password);
  SERIAL_PRINT(F("-->WiFi"));
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    SERIAL_PRINT(".");
  }
  
  //SERIAL_PRINTLN("");
  //SERIAL_PRINTLN("WiFi!");
  SERIAL_PRINT(F("IP Address: "));
  SERIAL_PRINTLN(WiFi.localIP());
}

#endif // WIFISETUP_H
