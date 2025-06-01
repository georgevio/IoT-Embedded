#include "websocket_client.h"
#include "esp_websocket_client.h" // ESP-IDF WebSocket client
#include "esp_log.h"
#include "secret.h" // For WEBSOCKET_URI

static const char* TAG = "WEBSOCKET_CLIENT";

static esp_websocket_client_handle_t client = NULL;
static bool websocket_connected_flag = false;

// Default timeout for WebSocket
#define ESP_WEBSOCKET_CLIENT_RETRY_MS_DEFAULT (10000)
#define ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS_DEFAULT (10000)


static void websocket_event_handler(void* handler_args, esp_event_base_t base, int32_t event_id, void* event_data)
{
    esp_websocket_event_data_t* data = (esp_websocket_event_data_t*)event_data;
    switch (event_id)
    {
    case WEBSOCKET_EVENT_CONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_CONNECTED");
        websocket_connected_flag = true;
        break;
    case WEBSOCKET_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DISCONNECTED");
        websocket_connected_flag = false;
        // Reconnection method is needed
        break;
    case WEBSOCKET_EVENT_DATA:
        ESP_LOGI(TAG, "WEBSOCKET_EVENT_DATA received");
        ESP_LOGI(TAG, "Received opcode=%d, data_len=%d", data->op_code, data->data_len);
        // client primarily sends data, handling of incoming data not interesting
        if (data->op_code == 0x08 && data->data_len == 2) { // Close frame
            ESP_LOGW(TAG, "Received closed message with code=%d", (data->data_ptr[0] << 8) | data->data_ptr[1]);
        }
        else if (data->data_ptr) {
            ESP_LOGW(TAG, "Received '%.*s'", data->data_len, (char*)data->data_ptr);
        }
        break;
    case WEBSOCKET_EVENT_ERROR:
        ESP_LOGE(TAG, "WEBSOCKET_EVENT_ERROR");
        websocket_connected_flag = false;
        // error logging is needed
        break;
    default:
        ESP_LOGI(TAG, "Unhandled WebSocket event (ID: %ld)", event_id);
        break;
    }
}

void websocket_client_start(void)
{
    if (client) {
        ESP_LOGW(TAG, "Initialized webSocket client stopping.");
        websocket_client_stop();
    }

    ESP_LOGI(TAG, "Starting WebSocket client to %s", WEBSOCKET_URI);

    esp_websocket_client_config_t websocket_cfg = {
        .uri = WEBSOCKET_URI,
        .reconnect_timeout_ms = ESP_WEBSOCKET_CLIENT_RETRY_MS_DEFAULT,
        .network_timeout_ms = ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS_DEFAULT,
        // For WSS (secure WebSocket), ensure ESP-TLS CA bundle is enabled in menuconfig
        // and .cert_pem can usually be NULL to use it.
        // Example for WSS:
        // .uri = "wss://echo.websocket.org",
        // .cert_pem = NULL, // Uses system CA bundle if configured in menuconfig
    };

    client = esp_websocket_client_init(&websocket_cfg);
    if (client == NULL) {
        ESP_LOGE(TAG, "WebSocket client initialization failed");
        return;
    }

    esp_err_t err = esp_websocket_register_events(client, WEBSOCKET_EVENT_ANY, websocket_event_handler, (void*)client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register WebSocket events: %s", esp_err_to_name(err));
        esp_websocket_client_destroy(client);
        client = NULL;
        return;
    }

    err = esp_websocket_client_start(client);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start WebSocket client: %s. Ensure network is up and URI is correct.", esp_err_to_name(err));
        // No need to destroy client here, start might clean up errors or caught by disconnect
    }
    else {
        ESP_LOGI(TAG, "WebSocket client start issued.");
    }
}

void websocket_client_stop(void)
{
    if (client)
    {
        ESP_LOGI(TAG, "Stopping WebSocket client...");
        esp_err_t err = esp_websocket_client_stop(client); //Gracefully stop the client
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to stop WebSocket client: %s", esp_err_to_name(err));
        }
        err = esp_websocket_client_destroy(client); //Destroy the client instance
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to destroy WebSocket client: %s", esp_err_to_name(err));
        }
        client = NULL;
        websocket_connected_flag = false;
        ESP_LOGI(TAG, "WebSocket client stopped and destroyed.");
    }
    else {
        ESP_LOGI(TAG, "WebSocket client not running or already destroyed.");
    }
}

esp_err_t websocket_send_frame(camera_fb_t* fb)
{
    if (!client) {
        ESP_LOGE(TAG, "WebSocket client not initialized.");
        return ESP_FAIL;
    }
    if (!websocket_connected_flag) {
        ESP_LOGE(TAG, "WebSocket client not connected.");
        return ESP_FAIL;
    }

    if (fb == NULL || fb->buf == NULL || fb->len == 0) {
        ESP_LOGE(TAG, "Invalid frame buffer provided.");
        return ESP_ERR_INVALID_ARG;
    }

    ESP_LOGI(TAG, "Sending frame: len=%u, format=%d", fb->len, fb->format);

    // esp_websocket_client_send_bin handles fragmentation if necessary.
    int bytes_sent = esp_websocket_client_send_bin(client, (const char*)fb->buf, fb->len, ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS_DEFAULT);
    if (bytes_sent < 0) {
        ESP_LOGE(TAG, "Error sending frame via WebSocket (error code: %d)", bytes_sent);
        return ESP_FAIL;
    }
    else if ((size_t)bytes_sent < fb->len) {
        ESP_LOGW(TAG, "Could not send full frame. Sent %d of %u bytes.", bytes_sent, fb->len);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Frame sent successfully (%d bytes).", bytes_sent);
    return ESP_OK;
}

bool is_websocket_connected(void) {
    return (client != NULL && websocket_connected_flag);
}