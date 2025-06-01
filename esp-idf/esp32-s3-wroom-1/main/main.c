/**
 * @file main.c
 * @brief Main application entry point
 * 
 * Initialize and coordinate mutliple modules (ON/OFF based on config.h).
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
 #include "bme280.h"
 
 static const char *TAG = "MAIN";
 static bool bme280_is_initialized = false;
 
 // MQTT connection state callback function
 static void on_mqtt_connection_change(bool connected, esp_mqtt_client_handle_t client)
 {
     if (connected && bme280_is_initialized) {
         ESP_LOGI(TAG, "MQTT connected, starting BME280 periodic readings...");
         esp_err_t ret = bme280_start_periodic_reading(client);
         if (ret != ESP_OK) {
             ESP_LOGE(TAG, "Failed to start BME280 periodic readings: %s", esp_err_to_name(ret));
         } else {
             ESP_LOGI(TAG, "BME280 periodic readings started successfully");
         }
     } else if (!connected) {
         ESP_LOGI(TAG, "MQTT disconnected, stopping BME280 readings");
         bme280_stop_periodic_reading();
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
     
     // Initialize BME280 sensor
     if (BME280_ENABLED) {
         ESP_LOGI(TAG, "Initializing BME280 sensor...");
         ret = bme280_init();
         if (ret == ESP_OK) {
             ESP_LOGI(TAG, "BME280 initialized");
             bme280_is_initialized = true;
             
             // Note: We don't start BME280 readings here
             // The MQTT connection callback will start readings when MQTT connects
             if (!MQTT_ENABLED || !wifi_is_connected()) {
                 ESP_LOGW(TAG, "MQTT not enabled or WiFi not connected, BME280 readings will not start");
             }
         } else {
             ESP_LOGE(TAG, "Failed to initialize BME280");
         }
     }

     // Main loop - publish only heartbeat  periodically if not using BME280
     // If BME280 is enabled, it handles its own publishing
     while (1) {
         if (MQTT_ENABLED && mqtt_client != NULL && mqtt_is_connected() && !BME280_ENABLED) {
             ESP_LOGI(TAG, "Publishing heartbeat message to AWS IoT...");
             
			 // Example publishing for default topic, check AWS IoT topics and rights
             mqtt_publish_message(mqtt_client, MQTT_TOPIC_DEVICE, "esp32-heartbeat", 0, 0);
         }
         
         // Sleep for the configured interval
         vTaskDelay(pdMS_TO_TICKS(BME280_SAMPLING_INTERVAL_MS));
     }
 }