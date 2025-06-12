// Abandoned: keeps on creating disconnections to the client after receiving fragments of frame


#include "esp_http_server.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>

// Required for vTaskDelay
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "websocket_server.h"
#include "config.h"

#ifndef WEBSOCKET_PORT
#define WEBSOCKET_PORT 80
#endif

static const char* TAG = "WEBSOCKET_SERVER";
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
    
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            return ESP_OK; 
        }
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %s", esp_err_to_name(ret));
        return ret;
    }
    
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (ws_pkt.len > 0) {
            buf = calloc(1, ws_pkt.len + 1);
            if (!buf) {
                ESP_LOGE(TAG, "Failed to calloc memory for text frame");
                return ESP_ERR_NO_MEM;
            }
            ws_pkt.payload = buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret != ESP_OK) {
                free(buf);
                return ret;
            }
            if (strcmp((char*)ws_pkt.payload, "{\"type\":\"heartbeat\"}") == 0) {
                ESP_LOGI(TAG, "Heartbeat received from fd %d", httpd_req_to_sockfd(req));
                const char* pong_msg = "{\"type\":\"heartbeat_ack\"}";
                websocket_server_send_text_client(httpd_req_to_sockfd(req), pong_msg);
            } else {
                 ESP_LOGI(TAG, "Received unknown msg from fd %d: %.*s", httpd_req_to_sockfd(req), ws_pkt.len, (char*)ws_pkt.payload);
            }
            free(buf);
        }
    } 
    else if (ws_pkt.type == HTTPD_WS_TYPE_BINARY) {
        size_t max_frame_size = 160000;
        uint8_t *frame_buf = NULL;
        size_t total_len = 0;

        if (ws_pkt.len > 0 && ws_pkt.len <= max_frame_size) {
            frame_buf = malloc(ws_pkt.len);
            if (!frame_buf) {
                ESP_LOGE(TAG, "Failed to allocate memory for the first frame fragment");
                return ESP_ERR_NO_MEM;
            }
            
            ws_pkt.payload = frame_buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret != ESP_OK) {
                free(frame_buf);
                ESP_LOGE(TAG, "Failed to receive the first frame payload: %s", esp_err_to_name(ret));
                return ret;
            }
            total_len = ws_pkt.len;
        }

        while (!ws_pkt.final) {
            ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
            if (ret != ESP_OK) {
                if (frame_buf) free(frame_buf);
                ESP_LOGE(TAG, "Failed to get info for next fragment: %s", esp_err_to_name(ret));
                return ret;
            }

            if (ws_pkt.type != HTTPD_WS_TYPE_CONTINUE) {
                 if (frame_buf) free(frame_buf);
                 ESP_LOGE(TAG, "Protocol error: Expected continuation frame, got %d", ws_pkt.type);
                 return ESP_FAIL;
            }

            if (ws_pkt.len > 0) {
                if (total_len + ws_pkt.len > max_frame_size) {
                    ESP_LOGE(TAG, "Frame exceeds max size limit");
                    if (frame_buf) free(frame_buf);
                    return ESP_ERR_NO_MEM;
                }

                uint8_t *temp_buf = realloc(frame_buf, total_len + ws_pkt.len);
                if (!temp_buf) {
                    ESP_LOGE(TAG, "Failed to reallocate memory for frame");
                    if (frame_buf) free(frame_buf);
                    return ESP_ERR_NO_MEM;
                }
                frame_buf = temp_buf;

                ws_pkt.payload = frame_buf + total_len;
                ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
                if (ret != ESP_OK) {
                    if (frame_buf) free(frame_buf);
                    ESP_LOGE(TAG, "Failed to receive next payload: %s", esp_err_to_name(ret));
                    return ret;
                }
                total_len += ws_pkt.len;
            }
        }
        
        ESP_LOGI(TAG, "Received complete frame from fd %d. Total Size: %d Bytes", httpd_req_to_sockfd(req), total_len);
        
        if (frame_buf) {
            free(frame_buf); 
        }

        ESP_LOGI(TAG, "Sending frame acknowledgement to fd %d", httpd_req_to_sockfd(req));
        const char* ack_msg = "{\"type\":\"frame_ack\"}";
        websocket_server_send_text_client(httpd_req_to_sockfd(req), ack_msg);
        
        // *** SOLUTION ***
        // Add a short delay to allow the TCP ACK to be sent and stabilize the connection
        // before the handler returns and polls the socket again.
        vTaskDelay(pdMS_TO_TICKS(100));
    }
    else if (ws_pkt.type == HTTPD_WS_TYPE_CLOSE) {
        ESP_LOGI(TAG, "Received CLOSE frame from fd %d", httpd_req_to_sockfd(req));
    }

    return ESP_OK;
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
    config.lru_purge_enable = true;
    config.stack_size = 8192; 
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    ESP_LOGI(TAG, "Starting Websocket server on port: '%d' with stack size %d", config.server_port, config.stack_size);
    
    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting Websocket server: %s", esp_err_to_name(ret));
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

    ESP_LOGI(TAG, "WebSocket server started ok!");
    return ESP_OK;
}

esp_err_t stop_websocket_server(void) {
    if (server_handle) {
        ESP_LOGI(TAG, "Stopping WebSocket server...");
        esp_event_handler_unregister(ESP_HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_DISCONNECTED, server_event_handler);
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

    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.payload = (uint8_t*)data_to_send;
    ws_pkt.len = strlen(data_to_send);
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.final = true;

    esp_err_t send_ret = httpd_ws_send_frame_async(server_handle, fd, &ws_pkt);
    if (send_ret != ESP_OK) {
        ESP_LOGE(TAG, "httpd_ws_send_frame_async failed for fd %d with error: %s (%d)", fd, esp_err_to_name(send_ret), send_ret);
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