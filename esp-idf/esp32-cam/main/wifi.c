#include "wifi.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
//#include "secret.h" // Contains WIFI_SSID and WIFI_PASSWORD , used in main.c

static const char *TAG_WIFI = "WIFI";

/**
 * @brief WiFi event handler.
 */
static void wifi_event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGW(TAG_WIFI, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG_WIFI, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

/**
 * @brief Initialize WiFi in Station mode.
 * 
 * // @return esp_err_t // Is there a need to return the error?
 */
//esp_err_t 
void wifi_init_sta(void) {
    esp_err_t ret;

    ESP_LOGI(TAG_WIFI, "Initializing WiFi...");

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_t *sta_netif = esp_netif_create_default_wifi_sta();
    assert(sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "WiFi initialization failed: %s", esp_err_to_name(ret));
        //return ret;
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
        ESP_LOGE(TAG_WIFI, "WiFi set mode failed: %s", esp_err_to_name(ret));
        //return ret;
    }

    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "WiFi set config failed: %s", esp_err_to_name(ret));
        //return ret;
    }

    ret = esp_wifi_start();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG_WIFI, "WiFi start failed: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG_WIFI, "WiFi initialized successfully.");
    }

    //return ret;
}
