#ifndef SENSORREADINGS_H
#define SENSORREADINGS_H

#include <DHT_nonblocking.h>
#include <Adafruit_BMP280.h>
#include "Config.h"

// Constants
#define DHTPIN 5
#define DHTTYPE DHT11

DHT_nonblocking dht_sensor(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

// Function to setup sensors
void setupSensors() {
  // DHT sensor doesn't require begin call, just initialization in constructor
  if (!bmp.begin()) {
    SERIAL_PRINTLN("Failed to start BMP280 sensor!");
    while (1);
  }

  SERIAL_PRINTLN("Sensors initialized successfully.");
}

// Function to read DHT sensor (non-blocking)
bool readDHTSensor(float *temperature, float *humidity) {
  unsigned long startTime = millis();
  static unsigned long lastTime = 0;

  if ((millis() - lastTime) >= 2000) { // Wait at least 2 seconds between measurements
    lastTime = millis();
    if (dht_sensor.measure(temperature, humidity)) {
      SERIAL_PRINT("DHT Humidity: ");
      SERIAL_PRINT(*humidity);
      SERIAL_PRINT(" %\tDHT Temperature: ");
      SERIAL_PRINT(*temperature);
      SERIAL_PRINTLN(" *C");
      return true;
    }
  }
  return false;
}

// Function to read BMP280 sensor
void readBMP280Sensor() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();

  SERIAL_PRINT("BMP280 Temperature: ");
  SERIAL_PRINT(temperature);
  SERIAL_PRINTLN(" *C");

  SERIAL_PRINT("BMP280 Pressure: ");
  SERIAL_PRINT(pressure);
  SERIAL_PRINTLN(" Pa");

  SERIAL_PRINT("Approx. Altitude: ");
  SERIAL_PRINT(bmp.readAltitude(1013.25)); // Adjust baseline pressure to your location
  SERIAL_PRINTLN(" m");
}

void readSensors() {
  float temperature, humidity;
  if (readDHTSensor(&temperature, &humidity)) {
    readBMP280Sensor();
  }
}

#endif // SENSORREADINGS_H
