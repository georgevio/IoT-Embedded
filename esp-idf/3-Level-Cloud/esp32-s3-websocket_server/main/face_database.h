/**
 * @file face_database.h
 * @brief Manages loading and accessing face metadata from storage.
 *
 * This is responsible for parsing the faces.json file on startup,
 * holding the face records in memory, and providing access to them.
 */

#ifndef FACE_DATABASE_H
#define FACE_DATABASE_H

#include "esp_err.h"
#include <stdbool.h>

#define MAX_NAME_LEN 32
#define MAX_TITLE_LEN 32
#define MAX_STATUS_LEN 16
#define MAX_FILENAME_LEN 64

// A single face record in memory
typedef struct {
    int id;
    int access_level;
    char name[MAX_NAME_LEN];
    char title[MAX_TITLE_LEN];
    char status[MAX_STATUS_LEN];
    char image_file[MAX_FILENAME_LEN];
    char embedding_file[MAX_FILENAME_LEN];
} face_record_t;

/**
 * @brief Initializes the face database.
 *
 * Reads and parses /spiffs/faces.json. If the file doesn't exist,
 * it creates an empty one. This is called after storage_init().
 *
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t database_init(void);

/**
 * @brief Deinitialize the database, free all allocated memory.
 */
void database_deinit(void);

/**
 * @brief Gets a pointer to the in-memory array of all face records.
 *
 * @param[out] out_faces Pointer to a face_record_t holding the array.
 * @param[out] out_count Pointer to the integer number of records.
 * @return esp_err_t ESP_OK on success.
 */
esp_err_t database_get_all_faces(face_record_t** out_faces, int* out_count);

#endif // FACE_DATABASE_H