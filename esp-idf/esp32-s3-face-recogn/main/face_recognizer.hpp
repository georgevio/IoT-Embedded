#pragma once

#include "dl_image_define.hpp"
#include "dl_detect_define.hpp"
#include "dl_recognition_define.hpp"
#include <list>
#include <vector>

/**
 * @class FaceRecognizer
 * @brief ESP-WHO libs for enrolling and recognizing raw images.
 * No FreeRTOS dependencies.
 */
class FaceRecognizer {
public:
    FaceRecognizer();
    ~FaceRecognizer();

    /**
     * @brief Port a face-image from a QQVGA buffer into the database.
     * @param image_buffer Pointer to the raw QQVGA (160x120) RGB888 image data.
     * @return The new ID of the enrolled face, or -1 on falure.
     */
    int enroll_face(uint8_t* image_buffer);

    /**
     * @brief Recognizes a face from a raw QQVGA image buffer.
     * @param image_buffer pointer to the QQVGA (160x120) RGB888 image to compare.
     * @return The ID of the recognized face, or -1 on failure (unidentified or unknown).
     */
    int recognize_face(uint8_t* image_buffer);

private:
    class HumanFaceDetect* m_detector;
    class HumanFaceRecognizer* m_recognizer;
};
