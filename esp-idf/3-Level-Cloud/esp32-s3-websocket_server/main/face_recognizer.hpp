#pragma once

#include <cstdint> 

// Forward declaration without need for the full definition.
class HumanFaceFeat;

class FaceRecognizer {
public:
    FaceRecognizer();
    ~FaceRecognizer();

    int recognize_face(uint8_t* image_buffer, int width, int height);

private:
    class HumanFaceDetect* m_detector;
    class HumanFaceRecognizer* m_recognizer;
    HumanFaceFeat* m_feat_model;
};