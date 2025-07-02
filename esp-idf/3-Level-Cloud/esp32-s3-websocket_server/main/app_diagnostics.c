#include <stdio.h>
#include <stdlib.h> // For free()
#include "esp_log.h"
#include "storage_manager.h"
#include "app_diagnostics.h"
#include "face_database.h"

static const char* TAG = "DIAGNOSTICS";

void diagnostics_run_storage_test(void) {
    const char* test_file_path = "/spiffs/test.txt";
    ESP_LOGI(TAG, "Running storage manager diagnostics...");

    esp_err_t write_err = storage_write_file(test_file_path, "Hello from the storage manager!");
    if (write_err != ESP_OK) {
        ESP_LOGE(TAG, "Storage test FAILED: Could not write to file.");
        return;
    }

    char* file_content = NULL;
    esp_err_t read_err = storage_read_file(test_file_path, &file_content);
    if (read_err != ESP_OK) {
        ESP_LOGE(TAG, "Storage test FAILED: Could not read from file (%s).", esp_err_to_name(read_err));
        return;
    }

    ESP_LOGI(TAG, "Read back from file: '%s'", file_content);
    free(file_content); // IMPORTANT: Free the buffer after use

    ESP_LOGI(TAG, "Storage manager diagnostics PASSED.");
}

void diagnostics_run_database_test(void) {
    ESP_LOGI(TAG, "Running database diagnostics...");

    face_record_t* faces = NULL;
    int count = 0;
    esp_err_t err = database_get_all_faces(&faces, &count);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Database test FAILED: Could not get faces.");
        return;
    }

    if (count == 0) {
        ESP_LOGI(TAG, "Database is empty, normal on first boot.");
    } else {
        ESP_LOGI(TAG, "Loaded %d faces from database", count);
        //Be careful, will print the whole database to terminal!
        //for (int i = 0; i < count; i++) {
        //    ESP_LOGI(TAG, "  - ID: %d, Name: %s, Title: %s", faces[i].id, faces[i].name, faces[i].title);
        //}
    }
    ESP_LOGI(TAG, "Database diagnostics PASSED.");
}