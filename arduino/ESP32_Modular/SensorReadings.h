#ifndef SENSORREADINGS_H
#define SENSORREADINGS_H

#include <DHT_Async.h>
#include <Adafruit_BMP280.h>
#include "Config.h"

// Constants
#define DHTPIN 5
#define DHTTYPE DHT_TYPE_11

DHT_Async dht_sensor(DHTPIN, DHTTYPE);
Adafruit_BMP280 bmp;

// Function to setup sensors
void setupSensors() {
  //dht_sensor.begin(); // No such method in DHT_Async
  
  if (!bmp.begin()) {
    SERIAL_PRINTLN(F("BMP280 failed!"));
    while (1);
  }

  SERIAL_PRINTLN(F("BMP280 on"));
}

bool readDHTSensor(float *temperature, float *humidity) { 
	static unsigned long lastTime = 0; 
	if ((millis() - lastTime) >= 2000) { 
		// Wait at least 2 seconds between measurements lastTime = millis(); 
		if (dht_sensor.measure(temperature, humidity)) { 		
			SERIAL_PRINT(F("Hum: ")); 
      SERIAL_PRINT(*humidity); 
			SERIAL_PRINT(F(" %\t Temp: ")); 
      SERIAL_PRINT(*temperature); 
			SERIAL_PRINTLN(F(" *C")); 
			return true;
		} 
	} 
	return false; 
}

// Function to read BMP280 sensor
void readBMP280Sensor() {
  float temperature = bmp.readTemperature();
  float pressure = bmp.readPressure();

  const char msg1[] = "BMP280 Temp: ";
  SERIAL_PRINT(F(msg1));
  SERIAL_PRINT(temperature);
  SERIAL_PRINTLN(F(" *C"));
  const char msg2[] = "BMP280 Press: ";
  SERIAL_PRINT(F(msg2));
  SERIAL_PRINT(pressure);
  SERIAL_PRINTLN(F(" Pa"));

  const char msg3[] = "Altitude: ";
  SERIAL_PRINT(F(msg3));
  SERIAL_PRINT(bmp.readAltitude(1013.25)); // Adjust baseline pressure to your location
  SERIAL_PRINTLN(" m");
}

void readSensors() {
  float temperature, humidity;
  readDHTSensor(&temperature, &humidity);
  readBMP280Sensor();
}

#endif // SENSORREADINGS_H
