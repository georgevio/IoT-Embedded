/**
 * @file mqtt.h
 * @brief MQTT client module for AWS IoT
 * Can be potenially used for other MQTT brokers. Credentials need to be created/updated.
 * Handles MQTT client initialization, connection, and publishing to AWS IoT.
 */

#ifndef MQTT_H
#define MQTT_H

#include "esp_err.h"
#include "mqtt_client.h"

/**
 * @brief Initialize MQTT client for AWS IoT
 * 
 * @return MQTT client handle on success, NULL on failure
 */
esp_mqtt_client_handle_t mqtt_aws_init(void);

/**
 * @brief Start the MQTT client
 * 
 * @param client MQTT client handle
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t mqtt_start(esp_mqtt_client_handle_t client);

/**
 * @brief Publish message to a specific topic
 * 
 * @param client MQTT client handle
 * @param topic MQTT topic to publish to
 * @param data Data to publish
 * @param qos Quality of Service (0, 1, or 2)
 * @param retain Retain flag
 * @return Message ID of the publish operation
 */
int mqtt_publish_message(esp_mqtt_client_handle_t client, const char *topic, const char *data, int qos, int retain);

/**
 * @brief Check if MQTT client is connected to broker
 * 
 * @return true if connected, false otherwise
 */
bool mqtt_is_connected(void);

/**
 * Register a callback function to be called when MQTT connection state changes
 * 
 * @param callback Function to call when connection state changes
 */
 void mqtt_register_connection_callback(void (*callback)(bool connected, esp_mqtt_client_handle_t client));

#endif // MQTT_H
