/**
 * @file main.c
 * @brief Main application entry point
 * 
 * Initializes and coordinates the different modules based on configuration.
 * 
 */

 #include <stdio.h>
 #include <string.h>
 #include <stdlib.h>  // For random number generation
 #include <time.h>    // For seeding the random number generator
 #include "nvs_flash.h"
 #include "esp_log.h"
 #include "esp_system.h"
 #include "freertos/FreeRTOS.h"
 #include "freertos/task.h"
 #include "config.h"
 #include "wifi.h"
 #include "mqtt.h"
 #include "camera.h"
 #include "esp_log.h"
 
 static const char *TAG = "MAIN";
 static bool esp32_cam_on = false;
 
 // MQTT connection state callback function
 static void on_mqtt_connection_change(bool connected, esp_mqtt_client_handle_t client)
 {
    if (connected) {
        ESP_LOGI(TAG, "MQTT connected OK...");
    } else {
        ESP_LOGW(TAG, "MQTT disconnected...");
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
    
    if (!camera_init()) {
        ESP_LOGE(TAG, "ESP32-CAM failed to initialize...");
        /* DISCUSSION: Do we want to return or initialize the rest?
         * for MQTT to start sending periodics, it checks esp32_cam_on and beacons failure...
         */
        //return; // Stop further execution if the camera didn't initialize properly
    } else {
        esp32_cam_on = true;
        ESP_LOGI(TAG, "ESP32-CAM init ok...");
    }

    // Initialize WiFi
    if (WIFI_ENABLED) {
        //ESP_LOGI(TAG, "Initializing WiFi...");
		wifi_init_sta(); // is there an if missing here?
        ESP_LOGI(TAG, "WiFi connected");
    }
    
    // Initialize MQTT if and when WiFi is connected
    esp_mqtt_client_handle_t mqtt_client = NULL;
    if (MQTT_ENABLED && wifi_is_connected()) {
        //ESP_LOGI(TAG, "Initializing MQTT...");
        
        // Register connection callback before initializing MQTT
        mqtt_register_connection_callback(on_mqtt_connection_change);
        
        mqtt_client = mqtt_aws_init();
        if (mqtt_client != NULL) {
            ESP_ERROR_CHECK(mqtt_start(mqtt_client));
            ESP_LOGI(TAG, "MQTT is ok");
            if(!esp32_cam_on){ // if the camera failed to init, but wifi & mqtt are on
                mqtt_publish_message(mqtt_client, AWS_IOT_CLIENT_ID, "ESP32-CAM FAILED!", 0, 0);
            }
        } else {
            ESP_LOGE(TAG, "MQTT failed to initialize");
        }
    }
    
    // Main eternal loop - publish messages periodically
    while (1) {
        //if (esp32_cam_on && MQTT_ENABLED && mqtt_client != NULL && mqtt_is_connected()) {
        if (MQTT_ENABLED && mqtt_client != NULL && mqtt_is_connected()) {
            ESP_LOGI(TAG, "Publishing ESP32-CAM random values to AWS IoT...");
            // Generate random values for the heartbeat message
            int temperature = rand() % 40;  // Random temperature between 0-39°C
            int humidity = rand() % 101;    // Random humidity between 0-100%
            int light_level = rand() % 1024; // Random light level between 0-1023
            
            // Create a message with random sensor values
            char message[100];
            snprintf(message, sizeof(message), 
                    "{\"temp\":%d,\"hum\":%d,\"light_level\":%d,\"status\":\"active\"}", 
                    temperature, humidity, light_level);
            
            // Publish the message with random values
            mqtt_publish_message(mqtt_client, TAG, message, 0, 0);
        
        }
        else{
            ESP_LOGE(TAG, "Failed to initialize component(s) in MAIN");
            return; // no point of working without all components up, right?
        }

        // Sleep for the configured interval
        vTaskDelay(pdMS_TO_TICKS(SAMPLING_INTERVAL_MS)); 
    }
 }