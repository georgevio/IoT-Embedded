#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "debug.h"
#include "esp_heap_caps.h"
#include <inttypes.h>

// Available log levels
#define LOG_NONE    ESP_LOG_NONE   // 1 - No logs
#define LOG_ERROR   ESP_LOG_ERROR  // 2 - Only errors
#define LOG_WARN    ESP_LOG_WARN   // 3 - Warnings and above (default)
#define LOG_INFO    ESP_LOG_INFO   // 4 - Informational logs
#define LOG_DEBUG   ESP_LOG_DEBUG  // 5 - Debug logs (most detailed)


// Set default log level (changing this will reflect in startup printout)
esp_log_level_t global_log_level = ESP_LOG_DEBUG;


void print_heap_info(const char *tag) {
    DEBUG_PRINT(tag, "Free internal heap: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_INTERNAL));
#ifdef CONFIG_ESP_PSRAM_ENABLE
    DEBUG_PRINT(tag, "Free PSRAM heap: %zu bytes", heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif
}

/**
 * @brief Convert log level to a human-readable string.
 */
const char* log_level_to_string(esp_log_level_t level) {
    switch (level) {
        case LOG_NONE:  return "NONE (1) - No logs";
        case LOG_ERROR: return "ERROR (2) - Only errors";
        case LOG_WARN:  return "WARN (3) - Warnings and above";
        case LOG_INFO:  return "INFO (4) - Informational logs";
        case LOG_DEBUG: return "DEBUG (5) - Most detailed";
        default:        return "UNKNOWN";
    }
}

/**
 * @brief Set global log level dynamically.
 */
void set_log_level(esp_log_level_t level) {
    global_log_level = level;

    // Apply log level to all components
    esp_log_level_set("*", level);         // Apply globally
    esp_log_level_set("wifi", level);      // Suppress excessive Wi-Fi logs
    esp_log_level_set("wifi_init", level); // Ensure Wi-Fi driver respects log level
    esp_log_level_set("esp_netif", level); // Suppress extra network interface logs
    esp_log_level_set("phy_init", level);  // Suppress unnecessary PHY logs
    esp_log_level_set("boot", level);      // Control boot logs
    esp_log_level_set("esp_image", level); // Control ESP-IDF image logs

    printf("\n\033[1;33m");  // Yellow color for visibility
    printf("=======================================================\n");
    printf("  Log level changed to: %s\n", log_level_to_string(level));
    printf("=======================================================\n");
    printf("\033[0m");  // Reset color
}



/**
 * @brief Handle runtime log level adjustment via serial input.
 */
void check_serial_input() {
    static bool prompt_shown = false;
    char input_buffer[10];

    if (!prompt_shown) {
        printf("\033[1;33m");  // Yellow color for clarity
        printf("\n=======================================================\n");
        printf("  Default log level: %s\n", log_level_to_string(global_log_level));
        printf("  Enter 'log 1-5' to set log level:\n");
        printf("     1 - LOG_NONE | 2 - LOG_ERROR | 3 - LOG_WARN (default)\n");
        printf("     4 - LOG_INFO | 5 - LOG_DEBUG\n");
        printf("=======================================================\n");
        printf("\033[0m");  // Reset color
        prompt_shown = true;
    }

    if (fgets(input_buffer, sizeof(input_buffer), stdin)) {
        int num_input = atoi(input_buffer); // Directly parse number input

        if (num_input >= 1 && num_input <= 5) {
            set_log_level(num_input);
        } else {
            printf("Unknown command. Use 'log 1-5' to change log level.\n");
        }
    }

}
