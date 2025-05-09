#include <ArduinoJson.h>

void setup() {
  Serial.begin(115200);
  StaticJsonDocument<200> doc;
  doc["sensor"] = "DHT11";
  doc["temperature"] = 24.5;

  String output;
  serializeJson(doc, output);
  Serial.println(output);
}

void loop() {
  // Nothing to do here
}
