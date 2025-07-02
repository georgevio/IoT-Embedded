#include <stdio.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_log.h"
#include "esp_spiffs.h"
#include "storage_manager.h"

static const char* TAG = "STORAGE_MANAGER";

esp_err_t storage_init(void) {
    ESP_LOGI(TAG, "Initializing SPIFFS");

    esp_vfs_spiffs_conf_t conf = {
      .base_path = "/spiffs",
      .partition_label = "storage", // Must match the name in partitions.csv
      .max_files = 5,               // Max number of files open at once
      .format_if_mount_failed = true
    };

    esp_err_t ret = esp_vfs_spiffs_register(&conf);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount or format filesystem");
        } else if (ret == ESP_ERR_NOT_FOUND) {
            ESP_LOGE(TAG, "Failed to find SPIFFS partition. Check partitions.csv.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize SPIFFS (%s)", esp_err_to_name(ret));
        }
        return ret;
    }

    size_t total = 0, used = 0;
    ret = esp_spiffs_info(conf.partition_label, &total, &used);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to get SPIFFS partition information (%s)", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "Partition size: total: %d, used: %d", total, used);
    }
    ESP_LOGI(TAG, "SPIFFS mounted successfully at %s", conf.base_path);
    return ESP_OK;
}

esp_err_t storage_write_file(const char* path, const char* data) {
    ESP_LOGD(TAG, "Writing to file: %s", path);
    FILE* f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, "%s", data);
    fclose(f);
    ESP_LOGD(TAG, "File written successfully");
    return ESP_OK;
}

esp_err_t storage_read_file(const char* path, char** buffer) {
    if (!storage_file_exists(path)) {
        return ESP_ERR_NOT_FOUND;
    }

    FILE* f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);

    *buffer = (char*)malloc(size + 1);
    if (*buffer == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for file content");
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    fread(*buffer, 1, size, f);
    fclose(f);
    (*buffer)[size] = '\0'; // Null-terminate the string

    return ESP_OK;
}

bool storage_file_exists(const char* path) {
    struct stat st;
    if (stat(path, &st) == 0) {
        return true;
    }
    return false;
}