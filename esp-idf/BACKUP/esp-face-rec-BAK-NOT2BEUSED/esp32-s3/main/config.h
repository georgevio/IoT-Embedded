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

#define SAMPLING_INTERVAL_MS 120000  // 2 minutes 

#endif // CONFIG_H