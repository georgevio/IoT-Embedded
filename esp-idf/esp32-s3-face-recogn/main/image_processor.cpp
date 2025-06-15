#include "image_processor.hpp"
#include "esp_log.h"
#include "esp_heap_caps.h"

// Image resolution for QQVGA (160x120) in RGB888 format from ESP32-CAM. Change at will...
#define IMAGE_WIDTH 160
#define IMAGE_HEIGHT 120
#define IMAGE_SIZE (IMAGE_WIDTH * IMAGE_HEIGHT * 3)

static const char *TAG = "IMG_PROCESS";

ImageProcessor::ImageProcessor() {
    m_face_recognizer = new FaceRecognizer();
}

ImageProcessor::~ImageProcessor() {
    delete m_face_recognizer;
}

uint8_t* ImageProcessor::create_dummy_image(uint8_t color) {
    uint8_t *image_buffer = (uint8_t *)heap_caps_malloc(IMAGE_SIZE, MALLOC_CAP_SPIRAM);
    if (image_buffer) {
        memset(image_buffer, color, IMAGE_SIZE);
    }
    return image_buffer;
}

// Dummy emulation of face recognition. Dummy images created, no real functionalites
void ImageProcessor::run_recognition_test() {
    ESP_LOGI(TAG, "========== START RECOGNITION ==========");

    // PORTING
    ESP_LOGI(TAG, "--- Porting faces ---");
    // Create a test "Person 1" (red image)
    uint8_t* person1_img = create_dummy_image(0xFF); // Red
    int person1_id = m_face_recognizer->enroll_face(person1_img);
    if(person1_id >= 0) ESP_LOGI(TAG, "Testing 'Person 1', ID: %d", person1_id);
    heap_caps_free(person1_img); // Free the buffer to be reused

    // Create a "Person 2" (green image)
    uint8_t* person2_img = create_dummy_image(0x80); // coloring the pic
    int person2_id = m_face_recognizer->enroll_face(person2_img);
    if(person2_id >= 0) ESP_LOGI(TAG, "Testing 'Person 2', ID: %d", person2_id);
    heap_caps_free(person2_img); 

    // RECOGNITION 
    ESP_LOGI(TAG, "--- Recognizing a face ---");
    // Create a new image from Person 2 to test
    uint8_t* test_img = create_dummy_image(0x80);
    int recognized_id = m_face_recognizer->recognize_face(test_img);
    
    ESP_LOGW(TAG, "Result: Trying to match 'Person 2'. Returned ID: %d", recognized_id);
    if(recognized_id == person2_id) {
        ESP_LOGI(TAG, "SUCCESS: correct ID was recognized.");
    } else {
        ESP_LOGE(TAG, "FAILED to recognize ID.");
    }
    heap_caps_free(test_img); // Free the buffer after use
    
    ESP_LOGI(TAG, "============ END RECOGNITION ============");
}
