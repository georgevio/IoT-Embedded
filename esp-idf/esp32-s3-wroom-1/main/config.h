/**
 * @file config.h
 * @brief Configuration for ESP32 application modules
 * 
 * Contains configuration flags to enable/disable various modules
 * and their respective settings.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "../certificates/secrets.h"  // Include sensitive configuration

// Module enable flags
#define WIFI_ENABLED 1
#define MQTT_ENABLED 1
#define BME280_ENABLED 1

// WiFi configuration
#define WIFI_MAXIMUM_RETRY 5

// BME280 configuration (if needed)
//#define BME280_I2C_ADDR 0x76  // Default I2C address for BME280 not working!
#define BME280_SDA_PIN 18     // Default SDA GPIO pin is 21
#define BME280_SCL_PIN 17     // Default SCL GPIO pin is 22
#define BME280_SAMPLING_INTERVAL_MS 120000  // 2 minutes - same as your publishing interval

#endif // CONFIG_H