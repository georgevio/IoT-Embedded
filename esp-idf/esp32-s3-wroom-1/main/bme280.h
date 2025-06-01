/**
 * @file bme280.h
 * @brief BME280 sensor module header
 * 
 * Handles BME280 sensor initialization and reading temperature, 
 * humidity, and pressure data.
 */

#ifndef BME280_H
#define BME280_H

#include "esp_err.h"

/**
 * @brief Structure to hold BME280 sensor readings
 */
typedef struct {
    float temperature;  // Temperature in Celsius
    float humidity;     // Relative humidity in percentage
    float pressure;     // Pressure in hPa (hectopascals)
} bme280_reading_t;

/**
 * @brief Initialize BME280 sensor
 * 
 * @return ESP_OK on success, specific error code otherwise
 */
esp_err_t bme280_init(void);

/**
 * @brief Read sensor data from BME280
 * 
 * @param reading Pointer to a bme280_reading_t structure to store the readings
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t bme280_read_data(bme280_reading_t *reading);

/**
 * @brief Start a task to periodically read BME280 sensor data and publish via MQTT
 * 
 * @param mqtt_client The MQTT client handle to use for publishing
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t bme280_start_periodic_reading(void *mqtt_client);

/**
 * @brief Stop the periodic reading task
 * 
 * @return ESP_OK on success, appropriate error code otherwise
 */
esp_err_t bme280_stop_periodic_reading(void);

#endif // BME280_H