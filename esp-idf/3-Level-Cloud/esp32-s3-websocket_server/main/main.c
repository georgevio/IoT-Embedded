/**
 * @file main.c
 * @brief Main application
 *
 * Multiple modules ON/OFF.
 */

#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h" 
#include "esp_netif.h" // get IP address

#include "config.h"
#include "wifi.h"

#if MQTT_ENABLED
#include "mqtt.h"
#endif

#if WEBSOCKET_ENABLED
#include "websocket_server.h"
#endif

static const char* TAG = "MAIN";

#if MQTT_ENABLED
// MQTT connection state callback function
static void on_mqtt_connection_change(bool connected, esp_mqtt_client_handle_t client) {
    if (connected) {
        ESP_LOGI(TAG, "MQTT connected...");
    }
    else if (!connected) {
        ESP_LOGI(TAG, "MQTT disconnected...");
    }
}
#endif // End of MQTT_ENABLED

void app_main(void) {
    // Logging
    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("WEBSOCKET_SERVER", ESP_LOG_VERBOSE);
    esp_log_level_set("HTTP_SERVER_WS", ESP_LOG_VERBOSE);
    esp_log_level_set(TAG, ESP_LOG_VERBOSE);

    ESP_LOGI(TAG, "Starting application...");

    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    // WiFi
    if (WIFI_ENABLED) { //
        ESP_LOGI(TAG, "Initializing WiFi...");
        wifi_init_sta(); //
        if (wifi_is_connected()) { //
            ESP_LOGI(TAG, "WiFi connected");
        }
        else {
            ESP_LOGE(TAG, "WiFi failed! Rest services will not start.");
        }
    }
    else {
        ESP_LOGE(TAG, "WiFi disabled. Rest services will not start.");
    }

#if MQTT_ENABLED 
    esp_mqtt_client_handle_t mqtt_client = NULL;
    // Initialize MQTT only if WiFi is enabled and connected
    if (WIFI_ENABLED && wifi_is_connected()) { //
        ESP_LOGI(TAG, "Initializing MQTT...");
        mqtt_register_connection_callback(on_mqtt_connection_change); //
        mqtt_client = mqtt_aws_init(); //
        if (mqtt_client != NULL) {
            ESP_ERROR_CHECK(mqtt_start(mqtt_client)); //
            ESP_LOGI(TAG, "MQTT initialized and started");
        }
        else {
            ESP_LOGE(TAG, "Failed to initialize MQTT");
        }
    }
    else { // if WIFI is not enabled
        ESP_LOGI(TAG, "MQTT not started, WiFi is not enabled or not connected.");
    }
#endif // End of MQTT_ENABLED

#if WEBSOCKET_ENABLED
    if (WIFI_ENABLED && wifi_is_connected()) { //
        ESP_LOGI(TAG, "Starting WebSocket Server...");
        ret = start_websocket_server(); //
        if (ret == ESP_OK) {
            esp_netif_ip_info_t ip_info;
            esp_netif_t* netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
            if (netif != NULL) {
                esp_netif_get_ip_info(netif, &ip_info);
                ESP_LOGI(TAG, "WebSocket server started...");
                // WebSocket URI path /ws in websocket_server.c
                // TWEBSOCKET_PORT in config.h
                ESP_LOGI(TAG, "WebSocket on: ws://" IPSTR ":%d/ws", IP2STR(&ip_info.ip), WEBSOCKET_PORT);
            }
            else {
                ESP_LOGE(TAG, "WiFi STA (netif) failed to display WebSocket URI.");
                ESP_LOGI(TAG, "WebSocket server started, but no IP address.");
                ESP_LOGI(TAG, "============ CONNECT TO THE IP ADDRESS ==================");
                ESP_LOGI(TAG, "WebSocket on at: ws://<ESP32_IP_ADDRESS>:%d/ws", WEBSOCKET_PORT); 
                ESP_LOGI(TAG, "=========================================================");
            }
        }
        else {
            ESP_LOGE(TAG, "WebSocket server failed: %s", esp_err_to_name(ret));
        }
    }
    else {
        ESP_LOGI(TAG, "WebSocket server can not start, WiFi is disabled.");
    }
#endif // End of WEBSOCKET_ENABLED

    // Main loop: not doing much currently, load periodic tasks here...
    while (1) {
        ESP_LOGD(TAG, "Main loop running...");
        vTaskDelay(pdMS_TO_TICKS(SAMPLING_INTERVAL_MS)); 
    }
}
