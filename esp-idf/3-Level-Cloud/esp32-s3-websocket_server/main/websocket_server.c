/* Code was taken from espidff examples and internet provided. Functionalities were used as-is */

#include "esp_http_server.h"
#include <esp_event.h>
#include <esp_log.h>
#include <esp_netif.h>
#include <esp_system.h>
#include <sys/param.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "websocket_server.h"
#include "config.h"
#include "cJSON.h" 

#ifndef WEBSOCKET_PORT
#define WEBSOCKET_PORT 80
#endif

static const char* TAG = "WEBSOCKET_SERVER";
#define MAX_WEBSOCKET_CLIENTS CONFIG_LWIP_MAX_ACTIVE_TCP

typedef struct {
    uint8_t *buffer;
    size_t total_size;
    size_t received_size;
    bool is_receiving;
    uint32_t id; // frame ID: maybe use some more advanced than int++, MAC ADDRESS?
} frame_receive_state_t;

typedef struct {
    int fd;
    bool active;
} ws_client_t;

static httpd_handle_t server_handle = NULL;
static ws_client_t ws_clients[MAX_WEBSOCKET_CLIENTS];
static frame_receive_state_t client_frame_states[MAX_WEBSOCKET_CLIENTS];

static void ws_async_send(void* arg);
static esp_err_t websocket_handler(httpd_req_t* req);
static void reset_client_frame_state(int fd);
static int find_client_index_by_fd(int fd);

static const httpd_uri_t ws_uri = {
    .uri = "/ws",
    .method = HTTP_GET,
    .handler = websocket_handler,
    .user_ctx = NULL,
    .is_websocket = true 
};

static int find_client_index_by_fd(int fd) {
    for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
        if (ws_clients[i].active && ws_clients[i].fd == fd) {
            return i;
        }
    }
    return -1;
}

static void reset_client_frame_state(int fd) {
    int client_index = find_client_index_by_fd(fd);
    if (client_index != -1) {
        if(client_frame_states[client_index].buffer) {
            free(client_frame_states[client_index].buffer);
        }
        client_frame_states[client_index].buffer = NULL;
        client_frame_states[client_index].is_receiving = false;
        client_frame_states[client_index].received_size = 0;
        client_frame_states[client_index].total_size = 0;
        client_frame_states[client_index].id = 0; // Reset ID
    }
}

static void server_event_handler(void* arg, esp_event_base_t event_base,
    int32_t event_id, void* event_data) {
    if (event_base == ESP_HTTP_SERVER_EVENT) { 
        if (event_id == HTTP_SERVER_EVENT_DISCONNECTED) { 
            int sockfd = *((int*)event_data);
            ESP_LOGI(TAG, "Client disconnected with fd %d", sockfd);
            reset_client_frame_state(sockfd);
            int client_index = find_client_index_by_fd(sockfd);
            if (client_index != -1) {
                ws_clients[client_index].active = false;
                ws_clients[client_index].fd = -1;
            }
        }
    }
}

static esp_err_t websocket_handler(httpd_req_t* req) {
    if (req->method == HTTP_GET) {
        ESP_LOGI(TAG, "Client connected with fd %d", httpd_req_to_sockfd(req));
        int sockfd = httpd_req_to_sockfd(req);
        int client_index = -1;
        for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
            if (!ws_clients[i].active) {
                client_index = i;
                break;
            }
        }

        if (client_index != -1) {
            ws_clients[client_index].fd = sockfd;
            ws_clients[client_index].active = true;
            reset_client_frame_state(sockfd);
            ESP_LOGI(TAG, "Client fd: %d added to list at index %d", sockfd, client_index);
            char welcome_msg[64];
            snprintf(welcome_msg, sizeof(welcome_msg), "Welcome, client fd %d!", sockfd);
            websocket_server_send_text_client(sockfd, welcome_msg);
        } else {
            ESP_LOGW(TAG, "Could not add client fd %d, client list full?", sockfd);
        }
        return ESP_OK;
    }

    httpd_ws_frame_t ws_pkt;
    uint8_t* buf = NULL;
    int client_index = find_client_index_by_fd(httpd_req_to_sockfd(req));

    if (client_index < 0) return ESP_FAIL;
    
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));

    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK) {
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) return ESP_OK; 
        return ret;
    }
    
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) {
        if (ws_pkt.len > 0) {
            buf = calloc(1, ws_pkt.len + 1);
            ws_pkt.payload = buf;
            ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
            if (ret != ESP_OK) {
                free(buf);
                return ret;
            }

            cJSON *root = cJSON_Parse((const char*)buf);
            if (root) {
                cJSON *type = cJSON_GetObjectItem(root, "type");
                if (cJSON_IsString(type)) {
                    if (strcmp(type->valuestring, "heartbeat") == 0) {
                        ESP_LOGI(TAG, "Heartbeat received from fd %d", httpd_req_to_sockfd(req));
                        const char* pong_msg = "{\"type\":\"heartbeat_ack\"}";
                        websocket_server_send_text_client(httpd_req_to_sockfd(req), pong_msg);

                    } else if (strcmp(type->valuestring, "frame_start") == 0) {
                        if (client_frame_states[client_index].is_receiving) {
                             reset_client_frame_state(httpd_req_to_sockfd(req));
                        }
                        cJSON *size = cJSON_GetObjectItem(root, "size");
                        // NEW: Get the ID from the JSON payload
                        cJSON *id = cJSON_GetObjectItem(root, "id");
                        if (cJSON_IsNumber(size) && cJSON_IsNumber(id)) {
                            client_frame_states[client_index].total_size = size->valueint;
                            client_frame_states[client_index].id = id->valueint; // Store the ID
                            client_frame_states[client_index].received_size = 0;
                            client_frame_states[client_index].buffer = malloc(size->valueint);
                            if (client_frame_states[client_index].buffer) {
                                client_frame_states[client_index].is_receiving = true;
                                ESP_LOGI(TAG, "Got frame_start for ID: %d, Size: %d", (int)id->valueint, (int)size->valueint);
                            } else {
                                ESP_LOGE(TAG, "Failed to allocate buffer!");
                            }
                        }
                    } else if (strcmp(type->valuestring, "frame_end") == 0) {
                         if (client_frame_states[client_index].is_receiving) {
                            if (client_frame_states[client_index].received_size == client_frame_states[client_index].total_size) {
                                // NEW: Log the ID on completion
                                ESP_LOGI(TAG, "Transfer complete for Frame ID: %d. Total size: %d",
                                    (int)client_frame_states[client_index].id, (int)client_frame_states[client_index].total_size);
                                const char* ack_msg = "{\"type\":\"frame_ack\"}";
                                websocket_server_send_text_client(httpd_req_to_sockfd(req), ack_msg);
                            } else {
                                ESP_LOGE(TAG, "Frame end for ID %d received, but size mismatch! Expected %d, got %d", 
                                    (int)client_frame_states[client_index].id,
                                    (int)client_frame_states[client_index].total_size, 
                                    (int)client_frame_states[client_index].received_size);
                            }
                         }
                         reset_client_frame_state(httpd_req_to_sockfd(req));
                    }
                }
                cJSON_Delete(root);
            }
            free(buf);
        }
    } 
    else if (ws_pkt.type == HTTPD_WS_TYPE_BINARY) {
        if (client_frame_states[client_index].is_receiving) {
            if (ws_pkt.len > 0 && (client_frame_states[client_index].received_size + ws_pkt.len <= client_frame_states[client_index].total_size)) {
                ws_pkt.payload = client_frame_states[client_index].buffer + client_frame_states[client_index].received_size;
                ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
                if (ret == ESP_OK) {
                    client_frame_states[client_index].received_size += ws_pkt.len;
                } else {
                    reset_client_frame_state(httpd_req_to_sockfd(req));
                }
            }
        } else {
            ESP_LOGW(TAG, "Received unexpected binary data from fd %d", httpd_req_to_sockfd(req));
        }
    }
    
    return ESP_OK;
}

esp_err_t start_websocket_server(void) {
    if (server_handle != NULL) return ESP_OK;

    for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
        ws_clients[i].active = false;
        ws_clients[i].fd = -1;
        memset(&client_frame_states[i], 0, sizeof(frame_receive_state_t));
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = WEBSOCKET_PORT;
    config.lru_purge_enable = true;
    config.stack_size = 8192; 
    config.recv_wait_timeout = 10;
    config.send_wait_timeout = 10;

    ESP_LOGI(TAG, "Websocket server on, port: '%d' with stack size %d", config.server_port, config.stack_size);
    
    esp_err_t ret = httpd_start(&server_handle, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Error starting Websocket server: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = httpd_register_uri_handler(server_handle, &ws_uri);
    if (ret != ESP_OK) {
        httpd_stop(server_handle);
        server_handle = NULL;
        return ret;
    }

    ESP_ERROR_CHECK(esp_event_handler_register(ESP_HTTP_SERVER_EVENT,
        HTTP_SERVER_EVENT_DISCONNECTED,
        server_event_handler,
        server_handle));

    ESP_LOGI(TAG, "WebSocket server up!");
    return ESP_OK;
}

esp_err_t stop_websocket_server(void) {
    if (server_handle) {
        esp_event_handler_unregister(ESP_HTTP_SERVER_EVENT, HTTP_SERVER_EVENT_DISCONNECTED, server_event_handler);
        httpd_stop(server_handle);
        server_handle = NULL;
    }
    return ESP_OK;
}

typedef struct {
    int fd;
    char* data;
} async_send_arg_t;

esp_err_t websocket_server_send_text_client(int fd, const char* data) {
    if (!server_handle) return ESP_FAIL;
    if (fd < 0) return ESP_ERR_INVALID_ARG;

    async_send_arg_t* task_arg = malloc(sizeof(async_send_arg_t));
    if (!task_arg) return ESP_ERR_NO_MEM;
    
    task_arg->fd = fd;
    task_arg->data = strdup(data);
    if (!task_arg->data) {
        free(task_arg);
        return ESP_ERR_NO_MEM;
    }

    if (httpd_queue_work(server_handle, ws_async_send, task_arg) != ESP_OK) {
        free(task_arg->data);
        free(task_arg);
        return ESP_FAIL;
    }
    return ESP_OK;
}

esp_err_t websocket_server_send_text_all(const char* data) {
    if (!server_handle) return ESP_FAIL;

    int active_clients_count = 0;
    for (int i = 0; i < MAX_WEBSOCKET_CLIENTS; i++) {
        if (ws_clients[i].active) {
            active_clients_count++;
            websocket_server_send_text_client(ws_clients[i].fd, data);
        }
    }

    if (active_clients_count == 0) return ESP_ERR_NOT_FOUND;
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

    httpd_ws_send_frame_async(server_handle, fd, &ws_pkt);

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
