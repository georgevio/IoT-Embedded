#include "face_recognizer.hpp"
#include "human_face_detect.hpp"
#include "human_face_recognition.hpp"
#include "esp_log.h"

static const char *TAG = "FACE_RECOGN";

FaceRecognizer::FaceRecognizer() {
    m_detector = new HumanFaceDetect();
    m_feat_model = new HumanFaceFeat();
    m_recognizer = new HumanFaceRecognizer(m_feat_model, (char*)"face.db");
    ESP_LOGI(TAG, "ESP-WHO libs Init.");
}

FaceRecognizer::~FaceRecognizer() {
    delete m_detector;
    delete m_recognizer;
    delete m_feat_model; // Clean up the feature model
    ESP_LOGI(TAG, "ESP-WHO libs unloaded (deleted objects).");
}

int FaceRecognizer::recognize_face(uint8_t *image_buffer, int width, int height) {
    dl::image::img_t image;
    image.width = width;
    image.height = height;
    image.data = image_buffer;
    image.pix_type = dl::image::DL_IMAGE_PIX_TYPE_RGB888;

    std::list<dl::detect::result_t> faces = m_detector->run(image);

    if (faces.empty()) {
        ESP_LOGI(TAG, "No face detected in image.");
        return -1;
    }
    std::vector<dl::recognition::result_t> results = m_recognizer->recognize(image, faces);

    if (results.empty()) {
        ESP_LOGI(TAG, "Unknown face detected (no match in dB).");
        return -1;
    }

    return results.front().id;
}