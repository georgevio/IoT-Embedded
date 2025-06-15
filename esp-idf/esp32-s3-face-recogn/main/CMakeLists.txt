idf_component_register(SRCS "app_main.cpp" 
                           "image_processor.cpp" 
                           "face_recognizer.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES "human_face_recognition" 
                                "human_face_detect" 
                                "esp-dl" 
                                "nvs_flash")
