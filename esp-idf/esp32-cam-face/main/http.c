#include "http.h"
#include "camera.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include <string.h>
#include "debug.h"  // Include centralized debug management

static const char *TAG_HTTP = "HTTP";

// Configuration Flags
#define ENABLE_STREAMING 1   
#define ENABLE_HTML_PAGE 1   

static httpd_handle_t server_handle = NULL;

/**
 * @brief Capture a single JPEG image.
 */
static esp_err_t jpg_capture_handler(httpd_req_t *req) {
    camera_fb_t *frame_buffer = NULL;
    esp_err_t ret = camera_capture(&frame_buffer);
    
    if (ret != ESP_OK || !frame_buffer) {
        DEBUG_PRINT(TAG_HTTP, "Failed to capture frame");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "image/jpeg");
    httpd_resp_send(req, (const char *)frame_buffer->buf, frame_buffer->len);
    
    camera_fb_return(frame_buffer);
    return ESP_OK;
}

/**
 * @brief Stream JPEG images from the camera as multipart content.
 */
static esp_err_t jpg_stream_handler(httpd_req_t *req) {
    char *boundary = "123456789000000000000987654321";
    char part_buf[128];  // Increased buffer size to 128 bytes

    httpd_resp_set_type(req, "multipart/x-mixed-replace; boundary=123456789000000000000987654321");

    while (1) {
        camera_fb_t *frame_buffer = NULL;
        esp_err_t ret = camera_capture(&frame_buffer);
        
        if (ret != ESP_OK || !frame_buffer) {
            DEBUG_PRINT(TAG_HTTP, "Failed to capture frame");
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }

        int header_len = snprintf(part_buf, sizeof(part_buf),
                                  "\r\n--%s\r\nContent-Type: image/jpeg\r\nContent-Length: %d\r\n\r\n",
                                  boundary, frame_buffer->len);
        
        if (header_len >= sizeof(part_buf)) {
            DEBUG_PRINT(TAG_HTTP, "Header buffer overflow");
            camera_fb_return(frame_buffer);
            continue;
        }

        ret = httpd_resp_send_chunk(req, part_buf, header_len);
        if (ret != ESP_OK) {
            camera_fb_return(frame_buffer);
            break;
        }

        ret = httpd_resp_send_chunk(req, (const char *)frame_buffer->buf, frame_buffer->len);
        camera_fb_return(frame_buffer);

        if (ret != ESP_OK) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(100));  // Adjust frame rate
    }

    httpd_resp_send_chunk(req, "\r\n--123456789000000000000987654321--\r\n", -1);
    return ESP_OK;
}

/**
 * @brief Serve a basic HTML page with links to static and streaming feeds.
 */
static esp_err_t index_html_handler(httpd_req_t *req) {
    const char *html_content = "<html>\
        <head><title>ESP32-CAM Control Panel</title></head>\
        <body>\
        <h1>ESP32-CAM Feed</h1>\
        <h2>Single Capture:</h2>\
        <img src=\"/capture\" width=\"320\" height=\"240\"><br><br>\
        <h2>Live Stream:</h2>\
        <img src=\"/stream\" width=\"320\" height=\"240\" style=\"display:block; margin:auto;\"><br>\
        </body>\
        </html>";

    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, html_content, strlen(html_content));
    return ESP_OK;
}

/**
 * @brief Initialize the HTTP server.
 */
void start_webserver(void) {
    DEBUG_PRINT(TAG_HTTP, "Starting HTTP server...");

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.task_priority = tskIDLE_PRIORITY + 1;

    esp_err_t err = httpd_start(&server_handle, &config);
    if (err == ESP_OK) {
        DEBUG_PRINT(TAG_HTTP, "HTTP server started.");

        httpd_uri_t index_uri = {
            .uri = "/",
            .method = HTTP_GET,
            .handler = index_html_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle, &index_uri);

        httpd_uri_t capture_uri = {
            .uri = "/capture",
            .method = HTTP_GET,
            .handler = jpg_capture_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle, &capture_uri);

        httpd_uri_t stream_uri = {
            .uri = "/stream",
            .method = HTTP_GET,
            .handler = jpg_stream_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server_handle, &stream_uri);

    } else {
        DEBUG_PRINT(TAG_HTTP, "Failed to start HTTP server: %s", esp_err_to_name(err));
    }
}
