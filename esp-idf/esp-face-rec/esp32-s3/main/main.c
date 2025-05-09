/**
 * @file main.c
 * @brief Main application entry point
 * 
 * Initializes and coordinates the different modules based on configuration.
 */

 #include <stdio.h>
 #include <string.h>
 #include "nvs_flash.h"
 #include "esp_log.h"
 #include "esp_system.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 
 #include "config.h"
 #include "wifi.h"
 #include "mqtt.h"
 
 static const char *TAG = "MAIN";
 
 // MQTT connection state callback function
 static void on_mqtt_connection_change(bool connected, esp_mqtt_client_handle_t client)
 {
     if (connected) {
         ESP_LOGI(TAG, "MQTT connected, you can start periodic readings...");

        // place holder for MQTT to send readings

     } else if (!connected) {
         ESP_LOGI(TAG, "MQTT disconnected...");

     }
 }
 
 void app_main(void)
 {
     // Initialize logging
     esp_log_level_set("*", ESP_LOG_VERBOSE);
     ESP_LOGI(TAG, "Starting application...");
     
     // Initialize NVS
     esp_err_t ret = nvs_flash_init();
     if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
         ESP_ERROR_CHECK(nvs_flash_erase());
         ESP_ERROR_CHECK(nvs_flash_init());
     }
     
     // Initialize WiFi
     if (WIFI_ENABLED) {
         ESP_LOGI(TAG, "Initializing WiFi...");
         wifi_init_sta();
         ESP_LOGI(TAG, "WiFi initialized and connected");
     }
     
     // Initialize MQTT if WiFi is connected
     esp_mqtt_client_handle_t mqtt_client = NULL;
     if (MQTT_ENABLED && wifi_is_connected()) {
         ESP_LOGI(TAG, "Initializing MQTT...");
         
         // Register connection callback before initializing MQTT
         mqtt_register_connection_callback(on_mqtt_connection_change);
         
         mqtt_client = mqtt_aws_init();
         if (mqtt_client != NULL) {
             ESP_ERROR_CHECK(mqtt_start(mqtt_client));
             ESP_LOGI(TAG, "MQTT initialized and started");
         } else {
             ESP_LOGE(TAG, "Failed to initialize MQTT");
         }
     }
     
     // Main eternal loop - publish messages periodically 
     while (1) {
         if (MQTT_ENABLED && mqtt_client != NULL && mqtt_is_connected()) {
             ESP_LOGI(TAG, "Publishing heartbeat message to AWS IoT...");
             
             // Example publishing for default topic
             mqtt_publish_message(mqtt_client, MQTT_TOPIC_DEVICE, "esp32-heartbeat", 0, 0);
         }
         
         // Sleep for the configured interval
         vTaskDelay(pdMS_TO_TICKS(SAMPLING_INTERVAL_MS)); 
     }
 }