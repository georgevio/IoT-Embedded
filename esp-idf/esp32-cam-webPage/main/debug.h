#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include "esp_log.h"

extern esp_log_level_t global_log_level;  // Make it accessible in main.c

// Log level definitions
#define LOG_NONE    ESP_LOG_NONE   // 1 - No logs
#define LOG_ERROR   ESP_LOG_ERROR  // 2 - Only errors
#define LOG_WARN    ESP_LOG_WARN   // 3 - Warnings and above (default)
#define LOG_INFO    ESP_LOG_INFO   // 4 - Informational logs
#define LOG_DEBUG   ESP_LOG_DEBUG  // 5 - Most detailed

void set_log_level(esp_log_level_t level);
void check_serial_input();
void print_heap_info(const char *tag);

// ANSI Color Codes for Readability
#define COLOR_RESET   "\033[0m"
#define COLOR_RED     "\033[1;31m"  // ERROR
#define COLOR_YELLOW  "\033[1;33m"  // WARNING
#define COLOR_GREEN   "\033[1;32m"  // INFO
#define COLOR_BLUE    "\033[1;34m"  // DEBUG

// Macros for Unified Logging (No Manual Updates Needed!)
#define DEBUG_PRINT(tag, fmt, ...) \
    do { \
        if (global_log_level >= LOG_DEBUG) { \
            printf("%s[DEBUG] %s: ", COLOR_BLUE, tag); \
            printf(fmt, ##__VA_ARGS__); \
            printf("%s\n", COLOR_RESET); \
        } \
    } while (0)

#define WARN_PRINT(tag, fmt, ...) \
    do { \
        if (global_log_level >= LOG_WARN) { \
            printf(COLOR_YELLOW "[WARNING] %s: " fmt COLOR_RESET "\n", tag, ##__VA_ARGS__); \
        } \
    } while (0)

#define ERROR_PRINT(tag, fmt, ...) \
    do { \
        printf(COLOR_RED "[ERROR] %s: " fmt COLOR_RESET "\n", tag, ##__VA_ARGS__); \
    } while (0)

#endif // DEBUG_H
