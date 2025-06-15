/**
 * @file config.h
 * @brief Configuration for ESP32 application modules
 * 
 * Configuration flags to enable/disable various modules
 * and their respective settings.
 */

#ifndef CONFIG_H
#define CONFIG_H

#include "../certificates/secret.h"  // Include sensitive configuration

// MQTT will try to connect to AWS with credentials from secret.h
#define MQTT_ENABLED 0

#define WIFI_ENABLED 1
#define WIFI_MAXIMUM_RETRY 5

#define WEBSOCKET_ENABLED 1
#define WEBSOCKET_PORT 80 

#define SAMPLING_INTERVAL_MS 120000  // 2 minutes for publishing interval

#endif // CONFIG_H