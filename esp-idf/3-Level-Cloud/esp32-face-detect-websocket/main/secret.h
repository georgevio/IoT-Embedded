#ifndef SECRET_H_
#define SECRET_H_

/* remember to change these for wifi connection */
#define WIFI_SSID "Casa Violetta" 
#define WIFI_PASSWORD "$$123789$$"

/* WebSocket Server Configuration */
// wss://socketsbay.com/wss/v2/1/demo/
#define WEBSOCKET_URI "https://echo.websocket.org" // Default public echo server for testing
#define WEBSOCKET_PORT 80 
#define ESP_WEBSOCKET_CLIENT_SEND_TIMEOUT_MS 2000 // Timeout for sending messages
#define ESP_WEBSOCKET_CLIENT_RETRY_MS 1000 // Timeout for sending messages


#endif /* SECRET_H_ */