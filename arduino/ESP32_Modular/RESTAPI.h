#ifndef RESTAPI_H
#define RESTAPI_H

#include <WebServer.h>
#include <ArduinoJson.h>
#include "Config.h"

WebServer server(80);

void handleRoot() {
  server.send(200, "text/html", "<h1>Hello, this is ESP32-S3!</h1>");
}

void handleStatus() {
  StaticJsonDocument<200> doc;
  doc["status"] = "OK";
  doc["uptime"] = millis() / 1000;

  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleNotFound() {
  server.send(404, "text/plain", "Not Found");
}

void setupRESTAPI() {
  server.on("/", handleRoot);
  server.on("/status", handleStatus);
  server.onNotFound(handleNotFound);
  server.begin();
  const char configMsg[] = "Web Server on";
  SERIAL_PRINTLN(F(configMsg));
}

void handleRESTAPI() {
  server.handleClient();
}

#endif // RESTAPI_H
