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
 static uint8_t BME280_I2C_ADDR = 0x77;  // Default, will be updated during init if needed

 // BME280 registers
 #define BME280_REG_CHIP_ID          0xD0
 #define BME280_REG_RESET            0xE0
 #define BME280_REG_CTRL_HUM         0xF2
 #define BME280_REG_STATUS           0xF3
 #define BME280_REG_CTRL_MEAS        0xF4
 #define BME280_REG_CONFIG           0xF5
 #define BME280_REG_PRESS_MSB        0xF7
 #define BME280_REG_PRESS_LSB        0xF8
 #define BME280_REG_PRESS_XLSB       0xF9
 #define BME280_REG_TEMP_MSB         0xFA
 #define BME280_REG_TEMP_LSB         0xFB
 #define BME280_REG_TEMP_XLSB        0xFC
 #define BME280_REG_HUM_MSB          0xFD
 #define BME280_REG_HUM_LSB          0xFE
 
 #define BME280_CHIP_ID              0x60
 
 // Calibration data
 typedef struct {
     uint16_t dig_T1;
     int16_t  dig_T2;
     int16_t  dig_T3;
     uint16_t dig_P1;
     int16_t  dig_P2;
     int16_t  dig_P3;
     int16_t  dig_P4;
     int16_t  dig_P5;
     int16_t  dig_P6;
     int16_t  dig_P7;
     int16_t  dig_P8;
     int16_t  dig_P9;
     uint8_t  dig_H1;
     int16_t  dig_H2;
     uint8_t  dig_H3;
     int16_t  dig_H4;
     int16_t  dig_H5;
     int8_t   dig_H6;
 } bme280_calib_data_t;
 
 static bme280_calib_data_t calib_data;
 static int32_t t_fine;
 
 // I2C functions
 static esp_err_t i2c_master_init(void)
 {
     int i2c_master_port = I2C_NUM_0;
     i2c_config_t conf = {
         .mode = I2C_MODE_MASTER,
         .sda_io_num = BME280_SDA_PIN,
         .scl_io_num = BME280_SCL_PIN,
         .sda_pullup_en = GPIO_PULLUP_ENABLE,
         .scl_pullup_en = GPIO_PULLUP_ENABLE,
         .master.clk_speed = 100000,  // 100 KHz
     };
     
     esp_err_t err = i2c_param_config(i2c_master_port, &conf);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "I2C config failed: %s", esp_err_to_name(err));
         return err;
     }
     
     err = i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);
     if (err != ESP_OK) {
         ESP_LOGE(TAG, "I2C driver install failed: %s", esp_err_to_name(err));
         return err;
     }
     
     ESP_LOGI(TAG, "I2C initialized successfully");
     return ESP_OK;
 }
 
static esp_err_t bme280_read_reg(uint8_t reg_addr, uint8_t *data, size_t len)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_I2C_ADDR << 1) | I2C_MASTER_READ, true);
    if (len > 1) {
        i2c_master_read(cmd, data, len - 1, I2C_MASTER_ACK);
    }
    i2c_master_read_byte(cmd, data + len - 1, I2C_MASTER_NACK);
    i2c_master_stop(cmd);
    
    // Increase timeout to 100ms for more reliable communication
    esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 100 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read register 0x%02x: %s", reg_addr, esp_err_to_name(ret));
    }
    
    // Add delay between I2C operations to prevent bus flooding
    vTaskDelay(1 / portTICK_PERIOD_MS);
    
    return ret;
}
 
 static esp_err_t bme280_write_reg(uint8_t reg_addr, uint8_t data)
 {
     i2c_cmd_handle_t cmd = i2c_cmd_link_create();
     
     i2c_master_start(cmd);
     i2c_master_write_byte(cmd, (BME280_I2C_ADDR << 1) | I2C_MASTER_WRITE, true);
     i2c_master_write_byte(cmd, reg_addr, true);
     i2c_master_write_byte(cmd, data, true);
     i2c_master_stop(cmd);
     
     esp_err_t ret = i2c_master_cmd_begin(I2C_NUM_0, cmd, 1000 / portTICK_PERIOD_MS);
     i2c_cmd_link_delete(cmd);
     
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "Failed to write to register 0x%02x: %s", reg_addr, esp_err_to_name(ret));
     }
     
     return ret;
 }
 

 static esp_err_t bme280_read_calibration_data(void)
 {
     esp_err_t ret;
     uint8_t reg_data;
     
     // Read each calibration register individually
     // Temperature trimming parameters
     uint8_t temp_msb, temp_lsb;
     
     // Read T1 (2 bytes at 0x88 and 0x89)
     ret = bme280_read_reg(0x88, &temp_lsb, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0x89, &temp_msb, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_T1 = (temp_msb << 8) | temp_lsb;
     
     // Read T2 (2 bytes at 0x8A and 0x8B)
     ret = bme280_read_reg(0x8A, &temp_lsb, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0x8B, &temp_msb, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_T2 = (int16_t)((temp_msb << 8) | temp_lsb);
     
     // Read T3 (2 bytes at 0x8C and 0x8D)
     ret = bme280_read_reg(0x8C, &temp_lsb, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0x8D, &temp_msb, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_T3 = (int16_t)((temp_msb << 8) | temp_lsb);
     
	 // Pressure trimming parameters (read a few to test). Compare with others for reliability!
     // Read P1 (2 bytes at 0x8E and 0x8F)
     ret = bme280_read_reg(0x8E, &temp_lsb, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0x8F, &temp_msb, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_P1 = (temp_msb << 8) | temp_lsb;
     
     // Read H1 (1 byte at 0xA1)
     ret = bme280_read_reg(0xA1, &reg_data, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_H1 = reg_data;
     
     // Read H2 (2 bytes at 0xE1 and 0xE2)
     ret = bme280_read_reg(0xE1, &temp_lsb, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0xE2, &temp_msb, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_H2 = (int16_t)((temp_msb << 8) | temp_lsb);
     
     // Read H3 (1 byte at 0xE3)
     ret = bme280_read_reg(0xE3, &reg_data, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_H3 = reg_data;
     
     // Read H4 and H5 (3 bytes at 0xE4, 0xE5, 0xE6 - shared)
     uint8_t e4, e5, e6;
     
     ret = bme280_read_reg(0xE4, &e4, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0xE5, &e5, 1);
     if (ret != ESP_OK) return ret;
     
     ret = bme280_read_reg(0xE6, &e6, 1);
     if (ret != ESP_OK) return ret;
     
     // H4 = (0xE4[7:0] << 4) | (0xE5[3:0])
     calib_data.dig_H4 = ((int16_t)e4 << 4) | (e5 & 0x0F);
     
     // H5 = (0xE6[7:0] << 4) | (0xE5[7:4])
     calib_data.dig_H5 = ((int16_t)e6 << 4) | (e5 >> 4);
     
     // Read H6 (1 byte at 0xE7)
     ret = bme280_read_reg(0xE7, &reg_data, 1);
     if (ret != ESP_OK) return ret;
     
     calib_data.dig_H6 = (int8_t)reg_data;
     
     // Print the calibration values read successfully
     ESP_LOGI(TAG, "Calibration data read (partial): T1=%u, T2=%d, T3=%d, P1=%u, H1=%u", 
              calib_data.dig_T1, calib_data.dig_T2, calib_data.dig_T3, 
              calib_data.dig_P1, calib_data.dig_H1);
     
     ESP_LOGI(TAG, "Humidity calibration: H1=%u, H2=%d, H3=%u, H4=%d, H5=%d, H6=%d", 
              calib_data.dig_H1, calib_data.dig_H2, calib_data.dig_H3,
              calib_data.dig_H4, calib_data.dig_H5, calib_data.dig_H6);
     
     // Get temperature working, set values if T1 is zero
     if (calib_data.dig_T1 == 0) {
         ESP_LOGW(TAG, "Using default calibration values for temperature");
         calib_data.dig_T1 = 27504;  // Typical value
         calib_data.dig_T2 = 26435;  // Typical value
         calib_data.dig_T3 = -1000;  // Typical value
     }
     
     return ESP_OK;
 }

 // Compensate temperature reading based on calibration data
 static float bme280_compensate_temperature(int32_t adc_T)
 {
     int32_t var1, var2;
     
     var1 = ((((adc_T >> 3) - ((int32_t)calib_data.dig_T1 << 1))) * 
             ((int32_t)calib_data.dig_T2)) >> 11;
             
     var2 = (((((adc_T >> 4) - ((int32_t)calib_data.dig_T1)) * 
             ((adc_T >> 4) - ((int32_t)calib_data.dig_T1))) >> 12) * 
             ((int32_t)calib_data.dig_T3)) >> 14;
             
     t_fine = var1 + var2;
     
     float temperature = (t_fine * 5 + 128) >> 8;
     return temperature / 100.0f;
 }
 
 // Compensate pressure reading based on calibration data
 static float bme280_compensate_pressure(int32_t adc_P)
 {
     int64_t var1, var2, p;
     
     var1 = ((int64_t)t_fine) - 128000;
     var2 = var1 * var1 * (int64_t)calib_data.dig_P6;
     var2 = var2 + ((var1 * (int64_t)calib_data.dig_P5) << 17);
     var2 = var2 + (((int64_t)calib_data.dig_P4) << 35);
     var1 = ((var1 * var1 * (int64_t)calib_data.dig_P3) >> 8) + 
            ((var1 * (int64_t)calib_data.dig_P2) << 12);
     var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib_data.dig_P1) >> 33;
     
     if (var1 == 0) {
         return 0; // Avoid division by zero
     }
     
     p = 1048576 - adc_P;
     p = (((p << 31) - var2) * 3125) / var1;
     var1 = (((int64_t)calib_data.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
     var2 = (((int64_t)calib_data.dig_P8) * p) >> 19;
     
     p = ((p + var1 + var2) >> 8) + (((int64_t)calib_data.dig_P7) << 4);
     return (float)p / 256.0f / 100.0f; // Return pressure in hPa
 }
 
 // Compensate humidity reading based on calibration data
 static float bme280_compensate_humidity(int32_t adc_H)
 {
     int32_t v_x1_u32r;
     
     v_x1_u32r = (t_fine - ((int32_t)76800));
     v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib_data.dig_H4) << 20) - 
                 (((int32_t)calib_data.dig_H5) * v_x1_u32r)) + ((int32_t)16384)) >> 15) * 
                 (((((((v_x1_u32r * ((int32_t)calib_data.dig_H6)) >> 10) * 
                 (((v_x1_u32r * ((int32_t)calib_data.dig_H3)) >> 11) + ((int32_t)32768))) >> 10) + 
                 ((int32_t)2097152)) * ((int32_t)calib_data.dig_H2) + 8192) >> 14));
                 
     v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * 
                 ((int32_t)calib_data.dig_H1)) >> 4));
                 
     v_x1_u32r = (v_x1_u32r < 0) ? 0 : v_x1_u32r;
     v_x1_u32r = (v_x1_u32r > 419430400) ? 419430400 : v_x1_u32r;
     
     return (float)(v_x1_u32r >> 12) / 1024.0f;
 }
 
 esp_err_t bme280_init(void)
 {
     ESP_LOGI(TAG, "Initializing BME280 sensor");
     
     // Initialize I2C
     esp_err_t ret = i2c_master_init();
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "I2C initialization failed");
         return ret;
     }
     
     // Check chip ID DYNAMICALLY - try both common I2C addresses
     uint8_t chip_id = 0;
     uint8_t addresses[] = {0x77, 0x76};  // Try both common addresses
     bool sensor_found = false;
     
     for (int i = 0; i < 2; i++) {
         ESP_LOGI(TAG, "Trying BME280 at address 0x%02x", addresses[i]);
         
         // Update the address for this attempt
         BME280_I2C_ADDR = addresses[i];
         
         // Try to read chip ID
         ret = bme280_read_reg(BME280_REG_CHIP_ID, &chip_id, 1);
         if (ret == ESP_OK) {
             ESP_LOGI(TAG, "BME280 chip ID: 0x%02x", chip_id);
             if (chip_id == BME280_CHIP_ID) {
                 ESP_LOGI(TAG, "BME280 found at address 0x%02x", addresses[i]);
                 sensor_found = true;
                 break;
             }
         }
         
         vTaskDelay(50 / portTICK_PERIOD_MS);  // Delay before trying next address
         ESP_LOGI(TAG, "BME280 reset completed, reading calibration data");
     }
     
     if (!sensor_found) {
         ESP_LOGE(TAG, "BME280 sensor not found at any address");
         return ESP_ERR_NOT_FOUND;
     }
     
     // Reset the sensor
     ret = bme280_write_reg(BME280_REG_RESET, 0xB6);
     if (ret != ESP_OK) return ret;
     
     // Wait for reset to complete
     vTaskDelay(5 / portTICK_PERIOD_MS);
     
     // Read calibration data
     ret = bme280_read_calibration_data();
     if (ret != ESP_OK) return ret;
     
     /* Configure the sensor*/
     // Set humidity oversampling to 1x
     ret = bme280_write_reg(BME280_REG_CTRL_HUM, 0x01);
     if (ret != ESP_OK) return ret;
     
     // Set temperature oversampling to 2x, pressure oversampling to 16x, and normal mode
     ret = bme280_write_reg(BME280_REG_CTRL_MEAS, 0x57); // 0x57 = 0b01010111
     if (ret != ESP_OK) return ret;
     
     // Set standby time to 1000ms and IIR filter to 16
     ret = bme280_write_reg(BME280_REG_CONFIG, 0x50); // 0x50 = 0b01010000
     if (ret != ESP_OK) return ret;
     
     bme280_initialized = true;
     ESP_LOGI(TAG, "BME280 initialized successfully");
     
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
     
     uint8_t data[8];
     esp_err_t ret = bme280_read_reg(BME280_REG_PRESS_MSB, data, 8);
     if (ret != ESP_OK) {
         ESP_LOGE(TAG, "Failed to read raw sensor data");
         return ret;
     }
     
     // Convert pressure, temperature, and humidity ADC values
     int32_t adc_P = ((uint32_t)data[0] << 12) | ((uint32_t)data[1] << 4) | ((uint32_t)data[2] >> 4);
     int32_t adc_T = ((uint32_t)data[3] << 12) | ((uint32_t)data[4] << 4) | ((uint32_t)data[5] >> 4);
     int32_t adc_H = ((uint32_t)data[6] << 8) | (uint32_t)data[7];
     
     ESP_LOGD(TAG, "Raw ADC readings - Temperature: %ld, Pressure: %ld, Humidity: %ld", 
              adc_T, adc_P, adc_H);
     
     // Calculate compensated values
     reading->temperature = bme280_compensate_temperature(adc_T);
     reading->pressure = bme280_compensate_pressure(adc_P);
     reading->humidity = bme280_compensate_humidity(adc_H);
     
     ESP_LOGI(TAG, "BME280 reading: %.2f °C, %.2f %%, %.2f hPa", 
              reading->temperature, reading->humidity, reading->pressure);
     
     return ESP_OK;
 }
 
 static void bme280_periodic_reading_task(void *pvParameters)
 {
     esp_mqtt_client_handle_t mqtt_client = (esp_mqtt_client_handle_t)pvParameters;
     bme280_reading_t reading;
     char mqtt_data[128]; // Larger buffer for JSON data
     
     while (1) {
         // Read BME280 data
         if (bme280_read_data(&reading) == ESP_OK) {
             // Only if MQTT is connected, try to publish the data
             if (mqtt_is_connected()) {
                 // Format and publish all readings in a single JSON message
                 snprintf(mqtt_data, sizeof(mqtt_data), 
                          "{\"temperature\":%.2f,\"humidity\":%.2f,\"pressure\":%.2f}",
                          reading.temperature, reading.humidity, reading.pressure);
                 
                 ESP_LOGI(TAG, "Publishing to topic %s: %s", MQTT_TOPIC_BME280, mqtt_data);
                 mqtt_publish_message(mqtt_client, MQTT_TOPIC_BME280, mqtt_data, 0, 0);
                 
                 // publish individual readings to separate topics as well for easier parsing
                 snprintf(mqtt_data, sizeof(mqtt_data), "%.2f", reading.temperature);
                 ESP_LOGI(TAG, "Publishing to topic %s/temperature: %s", MQTT_TOPIC_BME280, mqtt_data);
                 mqtt_publish_message(mqtt_client, MQTT_TOPIC_BME280 "/temperature", mqtt_data, 0, 0);
                 
                 snprintf(mqtt_data, sizeof(mqtt_data), "%.2f", reading.humidity);
                 ESP_LOGI(TAG, "Publishing to topic %s/humidity: %s", MQTT_TOPIC_BME280, mqtt_data);
                 mqtt_publish_message(mqtt_client, MQTT_TOPIC_BME280 "/humidity", mqtt_data, 0, 0);
                 
                 snprintf(mqtt_data, sizeof(mqtt_data), "%.2f", reading.pressure);
                 ESP_LOGI(TAG, "Publishing to topic %s/pressure: %s", MQTT_TOPIC_BME280, mqtt_data);
                 mqtt_publish_message(mqtt_client, MQTT_TOPIC_BME280 "/pressure", mqtt_data, 0, 0);
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
     
	 // Delete the task at the end of execution
     vTaskDelete(bme280_task_handle);
     bme280_task_handle = NULL;
     
     ESP_LOGI(TAG, "BME280 periodic reading task stopped");
     return ESP_OK;
 }