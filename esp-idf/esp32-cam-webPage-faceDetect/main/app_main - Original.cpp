#include "who_camera.h"
#include "who_human_face_detection.hpp"
//#include "app_wifi.h" NOT used anymore, using local Wi-Fi functions
#include "app_httpd.hpp"
#include "app_mdns.h"
#include "wifi.h" //local Wi-Fi functions

static QueueHandle_t xQueueAIFrame = NULL;
static QueueHandle_t xQueueHttpFrame = NULL;

extern "C" void app_main()
{
	//app_wifi_main(); // NOT used anymore, using local Wi-Fi functions
	wifi_init_sta(); // frfom local wifi.c

    xQueueAIFrame = xQueueCreate(2, sizeof(camera_fb_t *));
    xQueueHttpFrame = xQueueCreate(2, sizeof(camera_fb_t *));

    register_camera(PIXFORMAT_RGB565, FRAMESIZE_QVGA, 2, xQueueAIFrame);
    app_mdns_main();
    register_human_face_detection(xQueueAIFrame, NULL, NULL, xQueueHttpFrame);
    register_httpd(xQueueHttpFrame, NULL, true);
}
