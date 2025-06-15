/**
 * @file mqtt.c
 * @brief MQTT client module implementation for AWS IoT
 * Mostly standarized code provided for AWS connections. No alterations
 * Handles MQTT client initialization, connection, and publishing to AWS IoT.
 */

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_system.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_mac.h"
#include "mqtt.h"
#include "config.h"
#include "../certificates/secret.h" // Contains AWS_IOT_ENDPOINT, AWS_IOT_CLIENT_ID, MQTT_TOPIC_BASE


static const char *TAG = "MQTT";
static bool mqtt_connected_status = false;

// certificates mentioned in the CMakeLists.txt
extern const char _binary_AmazonRootCA1_pem_start[]; // asm("_binary_AmazonRootCA1_pem_start");
extern const char _binary_new_certificate_pem_start[]; // asm("_binary_new_certificate_pem_start");
extern const char _binary_new_private_key_start[]; // asm("_binary_new_private_key_start");

static void (*connection_state_callback)(bool connected, esp_mqtt_client_handle_t client) = NULL;

// function implementation
void mqtt_register_connection_callback(void (*callback)(bool connected, esp_mqtt_client_handle_t client)) {
    connection_state_callback = callback;
}

// Update the MQTT event handler function to use the callback
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    
    switch ((esp_mqtt_event_id_t)event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT connected to AWS IoT!");
            mqtt_connected_status = true;  // Update connection status
            
            // Send initialization message when connected
            char init_message[256];
            char client_id[64];
            
            // Get the client ID - Config value or MAC address as fallback
            #ifdef AWS_IOT_CLIENT_ID
                snprintf(client_id, sizeof(client_id), "%s", AWS_IOT_CLIENT_ID);
            #else
                // Get MAC address as client ID if not specified
                uint8_t mac[6];
                esp_efuse_mac_get_default(mac);
                snprintf(client_id, sizeof(client_id), "ESP32S3_%02X%02X%02X%02X%02X%02X", 
                         mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            #endif
            
            // Format initialization message - fixed timestamp format specifier
            snprintf(init_message, sizeof(init_message), 
                     "{\"message\":\"Device connected\",\"client_id\":\"%s\",\"topic\":\"%s\",\"timestamp\":%llu}",
                     client_id, MQTT_TOPIC_BASE, (unsigned long long)(esp_timer_get_time() / 1000));
            
            // Publish to a dedicated connection status topic
            ESP_LOGI(TAG, "Sending initialization message: %s", init_message);
            mqtt_publish_message(client, MQTT_TOPIC_BASE "/status/connect", init_message, 1, 0);
            
            // Call the connection callback if registered
            if (connection_state_callback != NULL) {
                connection_state_callback(true, client);
            }
            
            break;
            
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT disconnected");
            mqtt_connected_status = false;  // Update connection status
            
            // Call the connection callback if registered
            if (connection_state_callback != NULL) {
                connection_state_callback(false, client);
            }
            break;
            
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT message published, msg_id=%d", event->msg_id);
            break;
            
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT data received:");
            ESP_LOGI(TAG, "  Topic: %.*s", event->topic_len, event->topic);
            ESP_LOGI(TAG, "  Data: %.*s", event->data_len, event->data);
            break;
            
        case MQTT_EVENT_ERROR:
            ESP_LOGE(TAG, "MQTT error occurred");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
                ESP_LOGE(TAG, "  Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGE(TAG, "  Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
                ESP_LOGE(TAG, "  Last captured errno : %d (%s)", event->error_handle->esp_transport_sock_errno,
                         strerror(event->error_handle->esp_transport_sock_errno));
            }
            break;
            
        default:
            ESP_LOGI(TAG, "MQTT event id:%ld", (long)event_id);
            break;
    }
}

esp_mqtt_client_handle_t mqtt_aws_init(void)
{
    // Optional: Check if certificate is loaded
    ESP_LOGI(TAG, "Root CA Preview: %.20s", _binary_AmazonRootCA1_pem_start);
    
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker = {
            .address.uri = AWS_IOT_ENDPOINT,
            .verification.certificate = _binary_AmazonRootCA1_pem_start,
        },
        .credentials = {
            .authentication = {
                .certificate = _binary_new_certificate_pem_start,
                .key = _binary_new_private_key_start,
            },
            .client_id = AWS_IOT_CLIENT_ID
        },
        .session = {
            .last_will = {
                .topic = MQTT_TOPIC_STATUS,
                .msg = "offline",
                .qos = 1,
            }
        }
    };
    
    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "Failed to initialize MQTT client");
        return NULL;
    }
    
    esp_err_t err = esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register MQTT event handler: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(client);
        return NULL;
    }
    
    return client;
}

esp_err_t mqtt_start(esp_mqtt_client_handle_t client)
{
    if (client == NULL) {
        return ESP_ERR_INVALID_ARG;
    }
    
    esp_err_t err = esp_mqtt_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start MQTT client: %s", esp_err_to_name(err));
    } else {
        ESP_LOGI(TAG, "MQTT client started");
    }
    
    return err;
}

int mqtt_publish_message(esp_mqtt_client_handle_t client, const char *topic, const char *data, int qos, int retain)
{
    if (client == NULL || topic == NULL || data == NULL) {
        ESP_LOGE(TAG, "Invalid arguments for mqtt_publish_message");
        return -1;
    }
    
    ESP_LOGI(TAG, "Publishing to topic: %s", topic);
    ESP_LOGI(TAG, "Message data: %s", data);
    ESP_LOGI(TAG, "QoS: %d, Retain: %d", qos, retain);
    
    int msg_id = esp_mqtt_client_publish(client, topic, data, 0, qos, retain);
    if (msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish to topic %s", topic);
    } else {
        ESP_LOGI(TAG, "Published to topic %s, msg_id=%d", topic, msg_id);
    }
    
    return msg_id;
}


bool mqtt_is_connected(void)
{
    return mqtt_connected_status;
}
