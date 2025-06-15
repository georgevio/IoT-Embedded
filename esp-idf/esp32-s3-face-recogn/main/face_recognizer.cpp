#include "face_recognizer.hpp"
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp"
#include "esp_log.h"

static const char *TAG = "FACE_RECOGN";

// from esp-who libraries. BE VERY CAREFUL WITH THE VERSIONS!
FaceRecognizer::FaceRecognizer() {
    m_detector = new HumanFaceDetect();
    m_recognizer = new HumanFaceRecognizer((char*)"face.db"); // db needs to be created!
    ESP_LOGI(TAG, "ESP-WHO libs Init.");
}

// Free the memory when object is destroyed
FaceRecognizer::~FaceRecognizer() {
    delete m_detector;
    delete m_recognizer;
    ESP_LOGI(TAG, "ESP-WHO libs unloaded (deleted objects).");
}

int FaceRecognizer::enroll_face(uint8_t *image_buffer) {
    dl::image::img_t image;
    image.width = 160;
    image.height = 120;
    image.data = image_buffer;
    image.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

    std::list<dl::detect::result_t> faces = m_detector->run(image);

    if (faces.empty()) {
        ESP_LOGW(TAG, "No face found in incoming image.");
        return -1;
    }

    // Returns the ID of the new face
    int enrolled_id = m_recognizer->enroll(image, faces);
    ESP_LOGI(TAG, "Ported image result ID: %d", enrolled_id);
    return enrolled_id;
}

int FaceRecognizer::recognize_face(uint8_t *image_buffer) {
    dl::image::img_t image;
    image.width = 160;
    image.height = 120;
    image.data = image_buffer;
    image.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

    std::list<dl::detect::result_t> faces = m_detector->run(image);

    if (faces.empty()) {
        ESP_LOGI(TAG, "No face detected in image.");
        return -1;
    } 

    std::vector<dl::recognition::result_t> results = m_recognizer->recognize(image, faces);
    
    if (results.empty()) {
        ESP_LOGI(TAG, "UNknown face detected (no match in dB).");
        return -1;
    }
    
    // Return the ID of the best match if any
    return results.front().id;
}
