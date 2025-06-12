// ABANDONED: mutliple fragments of frame force server to disconnect the clinet

#include "websocket_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include <string.h>

#include "esp_websocket_client.h"
#include "esp_log.h"
#include "secret.h"

static const char* TAG = "WEBSOCK_CL";
static EventGroupHandle_t s_app_event_group = NULL;

#define WEBSOCKET_CONNECTED_BIT (1 << 1)
#define FRAME_ACK_BIT (1 << 2)

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
            if (s_app_event_group) xEventGroupClearBits(s_app_event_group, WEBSOCKET_CONNECTED_BIT | FRAME_ACK_BIT);
            break;
        case WEBSOCKET_EVENT_ERROR:
            ESP_LOGE(TAG, "WEBSOCKET_EVENT_ERROR");
            break;
        case WEBSOCKET_EVENT_DATA:
            ESP_LOGD(TAG, "Received opcode=%d, data_len=%d", data->op_code, data->data_len);
             if (data->op_code == 0x08 && data->data_len == 2) {
                ESP_LOGW(TAG, "Got closed message, code=%d", (data->data_ptr[0] << 8) | data->data_ptr[1]);
            } else if (data->op_code == 1 && data->data_ptr) { // Text frame
                if (strstr((const char*)data->data_ptr, "frame_ack") != NULL) {
                    ESP_LOGI(TAG, "Got frame ACK.");
                    if (s_app_event_group) xEventGroupSetBits(s_app_event_group, FRAME_ACK_BIT);
                } else {
                    ESP_LOGI(TAG, "Got: '%.*s'", data->data_len, (char*)data->data_ptr);
                }
            }
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

    esp_websocket_client_config_t websocket_cfg = {};
    websocket_cfg.uri = WEBSOCKET_URI;
    websocket_cfg.reconnect_timeout_ms = 5000;
    websocket_cfg.network_timeout_ms = 10000;
    websocket_cfg.buffer_size = 160 * 1024;

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
        if (s_app_event_group) xEventGroupClearBits(s_app_event_group, WEBSOCKET_CONNECTED_BIT | FRAME_ACK_BIT);
        ESP_LOGI(TAG, "WebSocket client stopped.");
    }
    xSemaphoreGive(client_mutex);
}

esp_err_t websocket_send_frame(const uint8_t* data, size_t len) {
    if (client_mutex == NULL) return ESP_FAIL;
    
    esp_err_t ret = ESP_FAIL;
    if (xSemaphoreTake(client_mutex, pdMS_TO_TICKS(5000)) != pdTRUE) {
        ESP_LOGE(TAG, "Failed to get client mutex to send frame.");
        return ESP_FAIL;
    }

    if (client && websocket_connected_flag) {
        if (data != NULL && len > 0) {
            
            ESP_LOGI(TAG, "Sending frame, total size %zu", len);

            int bytes_sent = esp_websocket_client_send_bin(client, (const char*)data, len, pdMS_TO_TICKS(20000));

            if (bytes_sent < 0) {
                ESP_LOGE(TAG, "Frame send error");
                ret = ESP_FAIL;
            } else if (bytes_sent == (int)len) {
                ESP_LOGI(TAG, "Frame delivered to transport buffer.");
                ret = ESP_OK;
            } else {
                ESP_LOGW(TAG, "Incomplete frame send: %d of %zu", bytes_sent, len);
                ret = ESP_FAIL;
            }
        } else {
            ESP_LOGE(TAG, "Invalid frame buffer.");
        }
    } else {
        ESP_LOGW(TAG, "WebSocket not connected, cannot send frame.");
    }
    
    xSemaphoreGive(client_mutex);
    return ret;
}
// --- END MODIFICATION ---

esp_err_t websocket_send_heartbeat(void) {
    ESP_LOGI(TAG, "Sending ping...");
    const char* heartbeat_msg = "{\"type\":\"heartbeat\"}";
    if (client) {
        esp_websocket_client_send_text(client, heartbeat_msg, strlen(heartbeat_msg), portMAX_DELAY);
    }
    return ESP_OK;
}