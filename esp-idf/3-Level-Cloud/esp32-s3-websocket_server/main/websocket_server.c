#include "esp_http_server.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h> // check if needed?
#include <esp_system.h>

#include <sys/param.h> // For MIN/MAX if used, check?
#include <string.h>    // For memset, strlen, strdup
#include <stdlib.h>    // For calloc, malloc, free

#include "websocket_server.h"
#include "config.h"

#ifndef WEBSOCKET_PORT
#define WEBSOCKET_PORT 80
#endif

static const char* TAG = "WEBSOCKET_SERVER";

// CONFIG_LWIP_MAX_ACTIVE_TCP is from sdkconfig;
#define MAX_WEBSOCKET_CLIENTS CONFIG_LWIP_MAX_ACTIVE_TCP

typedef struct {
    int fd;
    bool active;
} ws_client_t;

static ws_client_t ws_clients[MAX_WEBSOCKET_CLIENTS];
static httpd_handle_t server_handle = NULL;

static void ws_async_send(void* arg);

static esp_err_t websocket_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Client connected with fd %d", httpd_req_to_sockfd(req));
        int sockfd = httpd_req_to_sockfd(req);
        bool client_added = false;
        for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
            if (!ws_clients[i].active) {
                ws_clients[i].fd = sockfd;
                ws_clients[i].active = true;
                ESP_LOGI(TAG, "Client fd: %d added to list at index %d", sockfd, i);
                char welcome_msg[64];
                snprintf(welcome_msg, sizeof(welcome_msg), "Welcome, client fd %d!", sockfd);
                websocket_server_send_text_client(sockfd, welcome_msg);
                client_added = true;
                break;
            }
        }
        if (!client_added) {
            ESP_LOGW(TAG, "Could not add client fd %d, client list full?", sockfd);
        }
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t* buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        if (ret != ESP_ERR_HTTPD_RESP_SEND && ret != ESP_ERR_INVALID_STATE) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %s", esp_err_to_name(ret));
        }
        return ret;
    }
    ESP_LOGV(TAG, "frame len is %d for type %d", ws_pkt.len, ws_pkt.type);

    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT && ws_pkt.len > 0) {
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL) {
            ESP_LOGE(TAG, "Failed to calloc memory for WebSocket frame");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %s", esp_err_to_name(ret));
            free(buf);
            return ret;
        }
        // ws_pkt.payload is null-terminated by calloc + 1 and if text
        ESP_LOGI(TAG, "Received packet from fd %d with message: %s", httpd_req_to_sockfd(req), (char*)ws_pkt.payload);

        ESP_LOGI(TAG, "Echoing message back to client fd %d: %s", httpd_req_to_sockfd(req), (char*)ws_pkt.payload);
        ret = httpd_ws_send_frame(req, &ws_pkt);
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "httpd_ws_send_frame failed with %s", esp_err_to_name(ret));
        }
        free(buf);
    }
    else if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "Received CLOSE frame from fd %d", httpd_req_to_sockfd(req));
        // Respond with a CLOSE frame
        ws_pkt.len = 0; // No payload for close ack from server
        httpd_ws_send_frame(req, &ws_pkt); // Send back the close frame
    }

    return ret;
}

static const httpd_uri_t ws_uri = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true 
};

static void server_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    if (event_base == ESP_HTTP_SERVER_EVENT) { 
        if (event_id == HTTP_SERVER_EVENT_DISCONNECTED) { 
            int sockfd = *((int*)event_data);
            ESP_LOGI(TAG, "Client disconnected with fd %d", sockfd);
            for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
                if (ws_clients[i].active && ws_clients[i].fd == sockfd) {
                    ws_clients[i].active = false;
                    ws_clients[i].fd = -1;
                    ESP_LOGI(TAG, "Client fd: %d removed from list", sockfd);
                    break;
                }
            }
        }
    }
}

esp_err_t start_websocket_server(void) {
    if (server_handle != NULL) {
        ESP_LOGW(TAG, "WebSocket server already started");
        return ESP_OK;
    }

    for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
        ws_clients[i].active = false;
        ws_clients[i].fd = -1;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEBSOCKET_PORT;
    config.ctrl_port = config.server_port + 100;
    if (config.ctrl_port == config.server_port) {
        config.ctrl_port = config.server_port - 100 > 0 ? config.server_port - 100 : config.server_port + 1;
        if (config.ctrl_port == config.server_port) config.ctrl_port++;
    }
    config.task_priority = 5;

    int http_server_reported_max_allowed_user_sockets = 7;

    if (http_server_reported_max_allowed_user_sockets < 1) {
        ESP_LOGE(TAG, "HTTP server reported max allowed user sockets is less than 1.");
        return ESP_ERR_INVALID_STATE;
    }
    config.max_open_sockets = http_server_reported_max_allowed_user_sockets;
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting HTTP server on port: '%d' for WebSockets, ctrl_port: %d, max_open_sockets: %d (LWIP_MAX_SOCKETS=%d, HTTPD_MAX_ALLOWED=%d)",
        config.server_port, config.ctrl_port, config.max_open_sockets, CONFIG_LWIP_MAX_ACTIVE_TCP, http_server_reported_max_allowed_user_sockets);

    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting HTTP server: %s", esp_err_to_name(ret));
        return ret;
    }

    ESP_LOGI(TAG, "Registering URI for /ws");
    ret = httpd_register_uri_handler(server_handle, &ws_uri);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to register URI handler: %s", esp_err_to_name(ret));
        httpd_stop(server_handle);
        server_handle = NULL;
        return ret;
    }

    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTP_SERVER_EVENT,
        HTTP_SERVER_EVENT_DISCONNECTED,
        server_event_handler,
        server_handle));

    ESP_LOGI(TAG, "WebSocket server started successfully.");
    return ESP_OK;
}

esp_err_t stop_websocket_server(void) {
    if (server_handle) {
        ESP_LOGI(TAG, "Stopping WebSocket server...");
        esp_event_handler_unregister(ESP_HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_DISCONNECTED, server_event_handler); // Corrected
        httpd_stop(server_handle);
        server_handle = NULL;
        ESP_LOGI(TAG, "WebSocket server stopped.");
    }
    return ESP_OK;
}

typedef struct {
    int fd;
    char* data;
} async_send_arg_t;


esp_err_t websocket_server_send_text_client(int fd, const char* data) {
    if (!server_handle) {
        ESP_LOGE(TAG, "Server not started");
        return ESP_FAIL;
    }
    if (fd < 0) {
        ESP_LOGE(TAG, "Invalid client fd: %d", fd);
        return ESP_ERR_INVALID_ARG;
    }

    async_send_arg_t* task_arg = malloc(sizeof(async_send_arg_t));
    if (!task_arg) {
        ESP_LOGE(TAG, "Failed to allocate memory for async send task arg");
        return ESP_ERR_NO_MEM;
    }
    task_arg->fd = fd;
    task_arg->data = strdup(data);
    if (!task_arg->data) {
        ESP_LOGE(TAG, "Failed to duplicate string for async send");
        free(task_arg);
        return ESP_ERR_NO_MEM;
    }

    if (httpd_queue_work(server_handle, ws_async_send, task_arg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to queue async send work for fd %d", fd);
        free(task_arg->data);
        free(task_arg);
        return ESP_FAIL;
    }
    return ESP_OK;
}


esp_err_t websocket_server_send_text_all(const char* data) {
    if (!server_handle) {
        ESP_LOGE(TAG, "Server not started");
        return ESP_FAIL;
    }

    int active_clients_count = 0;
    for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
        if (ws_clients[i].active) {
            active_clients_count++;
            async_send_arg_t* task_arg = malloc(sizeof(async_send_arg_t));
            if (!task_arg) {
                ESP_LOGE(TAG, "Failed to allocate memory for async send task arg (client %d)", ws_clients[i].fd);
                continue;
            }
            task_arg->fd = ws_clients[i].fd;
            task_arg->data = strdup(data);
            if (!task_arg->data) {
                ESP_LOGE(TAG, "Failed to duplicate string for async send (client %d)", ws_clients[i].fd);
                free(task_arg);
                continue;
            }

            if (httpd_queue_work(server_handle, ws_async_send, task_arg) != ESP_OK) {
                ESP_LOGE(TAG, "Failed to queue async send work for client fd %d", ws_clients[i].fd);
                free(task_arg->data);
                free(task_arg);
            }
        }
    }

    if (active_clients_count == 0) {
        ESP_LOGI(TAG, "No active WebSocket clients to send data to.");
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}

static void ws_async_send(void* arg) {
    async_send_arg_t* send_arg = (async_send_arg_t*)arg;
    int fd = send_arg->fd;
    char* data_to_send = send_arg->data;

    ESP_LOGD(TAG, "Async send to fd %d: %s", fd, data_to_send);

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data_to_send;
    ws_pkt.len = strlen(data_to_send);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.final = true; // Ensure final is set

    esp_err_t send_ret = httpd_ws_send_frame_async(server_handle, fd, &ws_pkt);
    if (send_ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame_async failed for fd %d with error: %s (%d)", fd, esp_err_to_name(send_ret), send_ret);
        for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
            if (ws_clients[i].active && ws_clients[i].fd == fd) {
                ESP_LOGI(TAG, "Removing client fd %d due to send error.", fd);
                ws_clients[i].active = false;
                ws_clients[i].fd = -1;
                break;
            }
        }
    }
    else {
        ESP_LOGD(TAG, "Message sent successfully to fd %d", fd);
    }

    free(data_to_send);
    free(send_arg);
}

bool websocket_server_is_client_connected(void) {
    for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
        if (ws_clients[i].active) {
            return true;
        }
    }
    return false;
}