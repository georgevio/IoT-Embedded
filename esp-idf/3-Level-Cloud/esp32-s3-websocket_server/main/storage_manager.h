/**
 * @file storage_manager.h
 * @brief Manages all filesystem (SPIFFS) operations.
 *
 * Provides an interface for initializing the filesystem and performing
 * file read/writes, abstracting the SPIFFS implementation details
 */

#ifndef STORAGE_MANAGER_H
#define STORAGE_MANAGER_H

#include "esp_err.h"
#include <stdbool.h>

/**
 * @brief Initializes the SPIFFS filesystem.
 *
 * This function mounts the 'storage' partition defined in partitions.csv.
 * It should be called once on application startup.
 *
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t storage_init(void);

/**
 * @brief Writes (overwrites) data to a file on the filesystem.
 *
 * Will create the file if it doesn't exist or overwrite otherwise.
 *
 * @param path Full path to the file (e.g., "/spiffs/data.txt").
 * @param data Null-terminated string data to write.
 * @return esp_err_t ESP_OK on success, or an error code on failure.
 */
esp_err_t storage_write_file(const char* path, const char* data);

/**
 * @brief Reads the entire content of a file into a buffer.
 *
 * @note The caller is responsible for freeing the buffer.
 *
 * @param path Full path to the file.
 * @param buffer Pointer to a char pointer to be be allocated and filled with file content.
 * @return esp_err_t ESP_OK on success, ESP_ERR_NOT_FOUND if file does not exist, or other error code.
 */
esp_err_t storage_read_file(const char* path, char** buffer);

/**
 * @brief Checks if a file exists on the filesystem.
 *
 * @param path Full path to the file.
 * @return bool true if the file exists, false otherwise.
 */
bool storage_file_exists(const char* path);

#endif // STORAGE_MANAGER_H