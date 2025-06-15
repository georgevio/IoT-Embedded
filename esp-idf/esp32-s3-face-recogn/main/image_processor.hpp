#pragma once

#include "face_recognizer.hpp"

/**
 * @class ImageProcessor
 * @brief Connecting to, and testing face recognition.
 */
class ImageProcessor {
public:
    ImageProcessor();
    ~ImageProcessor();

    /**
     * @brief Runs a complete test: prots simulated faces and
     * tries to recognize one of them.
     */
    void run_recognition_test();

private:
    /**
     * @brief Creates a dummy QQVGA image for testing.
     * @param color, the color to fill the image with (dummy).
     * @return A pointer to the image buffer. The caller must free it.
     */
    uint8_t* create_dummy_image(uint8_t color);

    FaceRecognizer* m_face_recognizer;
};
