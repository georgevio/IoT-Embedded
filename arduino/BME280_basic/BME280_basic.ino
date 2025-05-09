#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

/* Define I2C pins for ESP32-S3-WROOM-1 */ 
#define SDA_PIN 18
#define SCL_PIN 17

Adafruit_BME280 bme;  // Create a BME280 object

void setup() {
    Serial.begin(115200);
    while (!Serial) { delay(10); }  // Wait for Serial to initialize
    Serial.println("Serial communication started...");
    
    Wire.begin(SDA_PIN, SCL_PIN); // Set custom I2C pins
    Serial.println("I2C initialized...");

    if (!bme.begin(0x76)) { // Try 0x77 if 0x76 doesn't work
        Serial.println("Could not find a valid BME280 sensor, check wiring!");
        while (1);
    }
    Serial.println("BME280 sensor detected...");
}

void loop() {
    Serial.println("Reading BME280 values...");
    
    float temperature = bme.readTemperature();
    float humidity = bme.readHumidity();
    float pressure = bme.readPressure() / 100.0F;
    
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.println(" °C");

    Serial.print("Humidity: ");
    Serial.print(humidity);
    Serial.println(" %");

    Serial.print("Pressure: ");
    Serial.print(pressure);
    Serial.println(" hPa");

    Serial.println("---------------------------");

    delay(10000);
}
