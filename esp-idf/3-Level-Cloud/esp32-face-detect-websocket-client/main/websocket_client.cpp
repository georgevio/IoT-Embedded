#include "websocket_client.h"

// Renamed to *.cpp to be able to include C++ libraries from ESP-IDF.

// 1. FreeRTOS.h must come first!
#include "freertos/FreeRTOS.h"

#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_websocket_client.h"
#include "esp_log.h"
#include "secret.h"


static const char* TAG = "WEBSOCKET_CLIENT";
static EventGroupHandle_t s_app_event_group = NULL;
#define WEBSOCKET_CONNECTED_BIT (1 << 1)

static SemaphoreHandle_t client_mutex = NULL;
static esp_websocket_client_handle_t client = NULL;
static bool websocket_connected_flag = false;

static void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data) {
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*)event_data;
    switch (event_id) {
        case WEBSOCKET_EVENT_CONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
            websocket_connected_flag = true;
            if (s_app_event_group) xEventGroupSetBits(s_app_event_group, WEBSOCKET_CONNECTED_BIT);
            break;
        case WEBSOCKET_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
            websocket_connected_flag = false;
            if (s_app_event_group) xEventGroupClearBits(s_app_event_group, WEBSOCKET_CONNECTED_BIT);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WEBSOCKET_EVENT_ERROR");
            websocket_connected_flag = false;
            if (s_app_event_group) xEventGroupClearBits(s_app_event_group, WEBSOCKET_CONNECTED_BIT);
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGI(TAG, "Received opcode=%d, data_len=%d", data->op_code, data->data_len);
            break;
        default:
            break;
    }
}

void websocket_client_start(EventGroupHandle_t event_group) {
    if (client_mutex == NULL) client_mutex = xSemaphoreCreateMutex();
    
    xSemaphoreTake(client_mutex, portMAX_DELAY);
    if (client) {
        esp_websocket_client_stop(client);
        esp_websocket_client_destroy(client);
        client = NULL;
    }
    s_app_event_group = event_group;
    ESP_LOGI(TAG, "Starting WebSocket client...");

    // Use C++ style init {} to zero-initialize all struct members.
    // A lot of issues with "missing initializer" warnings...
    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = WEBSOCKET_URI;
    websocket_cfg.reconnect_timeout_ms = ESP_WEBSOCKET_CLIENT_RETRY_MS;
    websocket_cfg.network_timeout_ms = ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS;

    client = esp_websocket_client_init(&websocket_cfg);
    esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*)client);
    esp_websocket_client_start(client);
    xSemaphoreGive(client_mutex);
}

void websocket_client_stop(void) {
    if (client_mutex == NULL) return;
    xSemaphoreTake(client_mutex, portMAX_DELAY);
    if (client) {
        esp_websocket_client_stop(client);
        esp_websocket_client_destroy(client);
        client = NULL;
        websocket_connected_flag = false;
        if (s_app_event_group) xEventGroupClearBits(s_app_event_group, WEBSOCKET_CONNECTED_BIT);
        ESP_LOGI(TAG, "WebSocket client stopped.");
    }
    xSemaphoreGive(client_mutex);
}

esp_err_t websocket_send_frame(camera_fb_t* fb) {
    if (client_mutex == NULL) {
        return ESP_FAIL;
    }
    esp_err_t ret = ESP_FAIL;
    if (xSemaphoreTake(client_mutex, pdMS_TO_TICKS(2000)) == pdTRUE) {
        if (client && websocket_connected_flag) {
            if (fb != NULL && fb->buf != NULL && fb->len > 0) {
                int bytes_sent = esp_websocket_client_send_bin(client, (const char*)fb->buf, fb->len, ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS);
                if (bytes_sent == fb->len) {
                    ret = ESP_OK;
                } else {
                    ESP_LOGE(TAG, "WebSocket send error or partial send (sent %d of %d)", bytes_sent, (int)fb->len);
                }
            } else {
                ESP_LOGE(TAG, "Invalid frame buffer provided.");
            }
        } else {
            ESP_LOGW(TAG, "WebSocket not connected, cannot send frame.");
        }
        xSemaphoreGive(client_mutex);
    } else {
        ESP_LOGE(TAG, "Failed to acquire client mutex to send frame.");
    }
    return ret;
}