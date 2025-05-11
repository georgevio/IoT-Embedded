#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include <inttypes.h>
#include "secret.h"   // Contains WIFI_SSID and WIFI_PASSWORD
#include "debug.h"    // Include centralized debug management

static const char *TAG_WIFI = "WIFI";

/**
 * @brief WiFi event handler with IP weblinks & QR generator
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT) {
        switch (event_id) {
            case WIFI_EVENT_STA_START:
                DEBUG_PRINT(TAG_WIFI, "WiFi station mode started, attempting connection...");
                esp_wifi_connect();  // Ensure connection attempt starts
                break;

            case WIFI_EVENT_STA_CONNECTED:
                DEBUG_PRINT(TAG_WIFI, "WiFi connected to AP, waiting for IP...");
                break;

            case WIFI_EVENT_STA_AUTHMODE_CHANGE:
                DEBUG_PRINT(TAG_WIFI, "WiFi authentication mode changed. Verify security settings.");
                break;

            case WIFI_EVENT_STA_DISCONNECTED:
                {
                    wifi_event_sta_disconnected_t *disconnected_event = (wifi_event_sta_disconnected_t *)event_data;
                    DEBUG_PRINT(TAG_WIFI, "WiFi disconnected! Reason code: %d", disconnected_event->reason);
                    
                    // Attempt reconnect unless authentication failed (wrong password)
                    if (disconnected_event->reason == WIFI_REASON_AUTH_FAIL) {
                        DEBUG_PRINT(TAG_WIFI, "Authentication failed. Check credentials and try again.");
                    } else {
                        DEBUG_PRINT(TAG_WIFI, "Reconnecting to WiFi...");
                        esp_wifi_connect();
                    }
                }
                break;

            default:
                DEBUG_PRINT(TAG_WIFI, "Unhandled WiFi event: %" PRIi32, event_id);
                break;
        }
    } 
    else if (event_base == IP_EVENT) {
        switch (event_id) {
            case IP_EVENT_STA_GOT_IP:
                {
                    ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

                    // Print raw IP info
                    DEBUG_PRINT(TAG_WIFI, "Obtained IP Address (raw): %lu", event->ip_info.ip.addr);

                    // Extract IP address
                    char ip_str[16];
                    snprintf(ip_str, sizeof(ip_str), IPSTR, IP2STR(&event->ip_info.ip));

                    // Generate URL
                    char stream_url[64];
                    snprintf(stream_url, sizeof(stream_url), "http://%s/stream", ip_str);

                    // Print network details
                    DEBUG_PRINT(TAG_WIFI, "Subnet Mask: %lu", event->ip_info.netmask.addr);
                    DEBUG_PRINT(TAG_WIFI, "Gateway: %lu", event->ip_info.gw.addr);

                    // Display clickable links
                    printf("\n\033[1;36m=======================================================\n");
                    printf("  🎥 ESP32-CAM Web Interface Available!\n");
                    printf("  - 📽️ Live Stream:   \033[4m%s\033[0m\n", stream_url);
                    printf("=======================================================\033[0m\n");
                }
                break;

            default:
                DEBUG_PRINT(TAG_WIFI, "Unhandled IP event: %" PRIi32, event_id);
                break;
        }
    }
}

/**
 * @brief Initialize WiFi in Station mode.
 */
void wifi_init_sta(void) {
    esp_err_t ret;

    DEBUG_PRINT(TAG_WIFI, "Initializing WiFi...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        DEBUG_PRINT(TAG_WIFI, "WiFi initialization failed: %s", esp_err_to_name(ret));
    }

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL);

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };

    ret = esp_wifi_set_mode(WIFI_MODE_STA);
    if (ret != ESP_OK) {
        DEBUG_PRINT(TAG_WIFI, "WiFi set mode failed: %s", esp_err_to_name(ret));
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        DEBUG_PRINT(TAG_WIFI, "WiFi set config failed: %s", esp_err_to_name(ret));
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        DEBUG_PRINT(TAG_WIFI, "WiFi start failed: %s", esp_err_to_name(ret));
    } else {
        DEBUG_PRINT(TAG_WIFI, "WiFi initialized successfully...");
    }
}
