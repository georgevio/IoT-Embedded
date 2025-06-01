/**
 * @file secret.h
 * @brief Sensitive configuration data
 * 
 * This file contains sensitive credentials and should not be committed to version control.
 */

#ifndef SECRET_H
#define SECRET_H

// WiFi configuration
#define WIFI_SSID "Casa Violetta Ext"
#define WIFI_PASSWORD "$$123789$$"

/* WebSocket Server Configuration */
// #define WEBSOCKET_URI "wss://socketsbay.com/wss/v2/1/demo/""
//#define WEBSOCKET_URI "https://echo.websocket.org" // Default public echo server for testing
#define WEBSOCKET_URI "ws://192.168.178.159:80/ws" // ESP32-S3 WebSocket server URI
#define WEBSOCKET_PORT 80 
#define ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS 2000 // Timeout for sending messages
#define ESP_WEBSOCKET_CLIENT_RETRY_MS 1000 // Timeout for sending messages

// AWS IoT configuration
#define AWS_IOT_ENDPOINT "mqtts://d06130382zzz729t7l6ho-ats.iot.eu-north-1.amazonaws.com"
// Optionally define a client ID, if not specified, MAC address will be used
#define AWS_IOT_CLIENT_ID "esp32-s3-mqtt"

// Base topic structure
#define MQTT_TOPIC_BASE "embed"                                       // Base topic
#define MQTT_TOPIC_DEVICE MQTT_TOPIC_BASE "/" AWS_IOT_CLIENT_ID       // Base topic with device ID
// Specific topic paths
#define MQTT_TOPIC_STATUS MQTT_TOPIC_DEVICE "/status"                 // Status topic
#define MQTT_TOPIC_BME280 MQTT_TOPIC_DEVICE "/bme280"                 // BME280 data

#endif // SECRET_H