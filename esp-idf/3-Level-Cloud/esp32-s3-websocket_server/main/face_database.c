#include <stdlib.h>
#include <string.h>
#include "esp_log.h"
#include "cJSON.h"
#include "storage_manager.h"
#include "face_database.h"

static const char* TAG = "FACE_DATABASE";
static const char* DB_PATH = "/spiffs/faces.json";

// Declare symbols for the start and end of the embedded faces.json file.
extern const uint8_t faces_json_start[] asm("_binary_faces_json_start");
extern const uint8_t faces_json_end[]   asm("_binary_faces_json_end");

// In-memory representation of the database
static struct {
    face_record_t* records;
    int count;
} s_db = { .records = NULL, .count = 0 };


static esp_err_t create_db_from_embedded(void) {
    ESP_LOGI(TAG, "Creating database file from embedded firmware data...");
    const size_t json_size = faces_json_end - faces_json_start;
    
    char* json_data = malloc(json_size + 1);
    if (!json_data) {
        ESP_LOGE(TAG, "Failed to allocate memory to create embedded db file!");
        return ESP_ERR_NO_MEM;
    }
    memcpy(json_data, faces_json_start, json_size);
    json_data[json_size] = '\0'; // Null-terminate

    esp_err_t err = storage_write_file(DB_PATH, json_data);
    free(json_data);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write embedded data to file!");
    }
    return err;
}


esp_err_t database_init(void) {
    if (s_db.records) {
        ESP_LOGW(TAG, "Database already initialized.");
        return ESP_OK;
    }

    char* json_string = NULL;
    bool must_recreate = false;

    esp_err_t err = storage_read_file(DB_PATH, &json_string);

    if (err == ESP_ERR_NOT_FOUND) {
        ESP_LOGW(TAG, "Database file not found. Will create from embedded.");
        must_recreate = true;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read database file (%s).", esp_err_to_name(err));
        return err;
    } else if (strlen(json_string) < 5) { // Check if file is empty or invalid ("[]")
        ESP_LOGW(TAG, "Existing database file is empty. Will recreate from embedded.");
        must_recreate = true;
    }

    // Free the buffer if it was allocated
    if (json_string) {
        free(json_string);
        json_string = NULL;
    }

    // If the file was missing or empty, create it now.
    if (must_recreate) {
        if (create_db_from_embedded() != ESP_OK) {
            return ESP_FAIL;
        }
    }

    // Now, read the file again. It's guaranteed to exist and should have content.
    err = storage_read_file(DB_PATH, &json_string);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read database file after potential creation.");
        return err;
    }

    // BE CAREFUL: It will print the whole database to terminal!
    //ESP_LOGI(TAG, "Final raw content read from %s:\n%s", DB_PATH, json_string);

    cJSON* root = cJSON_Parse(json_string);
    free(json_string);

    if (!root || !cJSON_IsArray(root)) {
        ESP_LOGE(TAG, "Failed to parse JSON or root is not an array.");
        cJSON_Delete(root);
        return ESP_FAIL;
    }

    s_db.count = cJSON_GetArraySize(root);
    ESP_LOGI(TAG, "Found %d face records in database.", s_db.count);

    if (s_db.count == 0) {
        ESP_LOGW(TAG, "Database is still empty after attempting to create it. Check embedded file.");
        s_db.records = NULL;
        cJSON_Delete(root);
        return ESP_OK;
    }

    s_db.records = calloc(s_db.count, sizeof(face_record_t));
    if (!s_db.records) {
        ESP_LOGE(TAG, "Failed to allocate memory for %d records!", s_db.count);
        s_db.count = 0;
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    cJSON* elem = NULL;
    int i = 0;
    cJSON_ArrayForEach(elem, root) {
        s_db.records[i].id = cJSON_GetObjectItem(elem, "id")->valueint;
        s_db.records[i].access_level = cJSON_GetObjectItem(elem, "access_level")->valueint;
        strncpy(s_db.records[i].name, cJSON_GetObjectItem(elem, "name")->valuestring, MAX_NAME_LEN - 1);
        strncpy(s_db.records[i].title, cJSON_GetObjectItem(elem, "title")->valuestring, MAX_TITLE_LEN - 1);
        strncpy(s_db.records[i].status, cJSON_GetObjectItem(elem, "status")->valuestring, MAX_STATUS_LEN - 1);
        strncpy(s_db.records[i].image_file, cJSON_GetObjectItem(elem, "image_file")->valuestring, MAX_FILENAME_LEN - 1);
        strncpy(s_db.records[i].embedding_file, cJSON_GetObjectItem(elem, "embedding_file")->valuestring, MAX_FILENAME_LEN - 1);
        i++;
    }

    cJSON_Delete(root);
    return ESP_OK;
}

void database_deinit(void) {
    if (s_db.records) {
        free(s_db.records);
        s_db.records = NULL;
    }
    s_db.count = 0;
    ESP_LOGI(TAG, "Database deinitialized.");
}

esp_err_t database_get_all_faces(face_record_t** out_faces, int* out_count) {
    if (s_db.count > 0 && !s_db.records) {
        ESP_LOGE(TAG, "Database not initialized or in inconsistent state.");
        return ESP_FAIL;
    }
    *out_faces = s_db.records;
    *out_count = s_db.count;
    return ESP_OK;
}