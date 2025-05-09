#include "Config.h"
#include "WiFiSetup.h"
#include "BLESetup.h"
#include "RESTAPI.h"
#include "SensorReadings.h"
#include "CameraFeed.h"

void setup() {
  if (ENABLE_SERIAL_OUTPUT) {
    Serial.begin(115200);
    delay(500);
  }

  connectToWiFi();
  setupBLE();
  setupRESTAPI();
  setupSensors();
  setupCamera();
}

void loop() {
  handleRESTAPI();
  readSensors();
  handleCameraFeed();
  delay(1000); // Adjust as needed
}
