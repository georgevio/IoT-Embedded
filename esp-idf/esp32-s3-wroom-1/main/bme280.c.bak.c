/**
 * @file bme280.c
 * @brief BME280 sensor module implementation
 * 
 * Handles BME280 sensor initialization and reading temperature, 
 * humidity, and pressure data.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_err.h"
#include "mqtt_client.h"

#include "bme280.h"
#include "mqtt.h"
#include "config.h"

static const char *TAG = "BME280";
static TaskHandle_t bme280_task_handle = NULL;
static bool bme280_initialized = false;

// Mock data for now - to be implemented with actual BME280 sensor code
esp_err_t bme280_init(void)
{
    ESP_LOGI(TAG, "Initializing BME280 sensor");
    
    // TODO: Implement actual BME280 sensor initialization
    // For now, just set as initialized
    bme280_initialized = true;
    
    return ESP_OK;
}

esp_err_t bme280_read_data(bme280_reading_t *reading)
{
    if (!bme280_initialized) {
        ESP_LOGE(TAG, "BME280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (reading == NULL) {
        ESP_LOGE(TAG, "Reading pointer is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    // TODO: Implement actual BME280 sensor reading
    // For now, return mock data
    reading->temperature = 22.5f;  // Mock temperature in Celsius
    reading->humidity = 45.0f;     // Mock humidity percentage
    reading->pressure = 1013.25f;  // Mock pressure in hPa
    
    ESP_LOGI(TAG, "BME280 reading: %.2f °C, %.2f %%, %.2f hPa", 
             reading->temperature, reading->humidity, reading->pressure);
    
    return ESP_OK;
}

static void bme280_periodic_reading_task(void *pvParameters)
{
    esp_mqtt_client_handle_t mqtt_client = (esp_mqtt_client_handle_t)pvParameters;
    bme280_reading_t reading;
    char mqtt_data[32];
    
    while (1) {
        // Read BME280 data
        if (bme280_read_data(&reading) == ESP_OK) {
            // If MQTT is connected, publish the data
            if (mqtt_is_connected()) {
                // Format and publish temperature data
                snprintf(mqtt_data, sizeof(mqtt_data), "%.2f C", reading.temperature);
                mqtt_publish_message(mqtt_client, MQTT_TOPIC_BME280, mqtt_data, 0, 0);
                
                // You can extend this to publish humidity and pressure as well
                // on separate topics if needed
            } else {
                ESP_LOGW(TAG, "MQTT not connected, skipping publishing");
            }
        } else {
            ESP_LOGE(TAG, "Failed to read BME280 data");
        }
        
        // Wait for the next sampling interval
        vTaskDelay(BME280_SAMPLING_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

esp_err_t bme280_start_periodic_reading(void *mqtt_client)
{
    if (!bme280_initialized) {
        ESP_LOGE(TAG, "BME280 not initialized");
        return ESP_ERR_INVALID_STATE;
    }
    
    if (mqtt_client == NULL) {
        ESP_LOGE(TAG, "MQTT client handle is NULL");
        return ESP_ERR_INVALID_ARG;
    }
    
    if (bme280_task_handle != NULL) {
        ESP_LOGW(TAG, "Periodic reading task already running");
        return ESP_OK;
    }
    
    // Create the periodic reading task
    BaseType_t xReturned = xTaskCreate(
        bme280_periodic_reading_task,
        "bme280_task",
        4096,
        mqtt_client,  // Pass the MQTT client as a parameter
        5,
        &bme280_task_handle
    );
    
    if (xReturned != pdPASS) {
        ESP_LOGE(TAG, "Failed to create BME280 task");
        return ESP_FAIL;
    }
    
    ESP_LOGI(TAG, "BME280 periodic reading task started");
    return ESP_OK;
}

esp_err_t bme280_stop_periodic_reading(void)
{
    if (bme280_task_handle == NULL) {
        ESP_LOGW(TAG, "Periodic reading task not running");
        return ESP_OK;
    }
    
    // Delete the task
    vTaskDelete(bme280_task_handle);
    bme280_task_handle = NULL;
    
    ESP_LOGI(TAG, "BME280 periodic reading task stopped");
    return ESP_OK;
}