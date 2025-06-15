/**
 * @file wifi.h
 * @brief WiFi connectivity header
 * 
 * WiFi station mode initialization and connection management.
 */

#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"
#include "freertos/event_groups.h"

/**
 * @brief Event group handle for WiFi events
 */
extern EventGroupHandle_t wifi_event_group;

/**
 * @brief Bit flag for WiFi connection status
 */
extern const int WIFI_CONNECTED_BIT;

/**
 * @brief Initialize WiFi as a station and connect to the access point via configuration parameters (SSID, passowrd)
 * 
 * Will block until there is a WiFi connection
 */
void wifi_init_sta(void);

/**
 * @brief Check if WiFi is already connected
 * 
 * @return true if connected, false otherwise
 */
bool wifi_is_connected(void);

#endif // WIFI_H
