// George: File altered on May 29, 2025
#include "who_ai_utils.hpp"

#include "esp_log.h"
#include "esp_camera.h"

#include "dl_image.hpp"
#include "dl_define.hpp" // George:it usually contains DL_MAX and DL_MIN. 
                         // If DL_MIN is not found, you might need to define it:
                         // #ifndef DL_MIN
                         // #define DL_MIN(a,b) (((a)<(b))?(a):(b))
                         // #endif

static const char* TAG = "ai_utils";

// George: Table for RGB color reference (original)
// +-------+--------------------+----------+
// |       |        RGB565      |  RGB888  |
// +=======+====================+==========+
// |  Red  | 0b0000000011111000 | 0x0000FF |
// +-------+--------------------+----------+
// | Green | 0b1110000000000111 | 0x00FF00 |
// +-------+--------------------+----------+
// |  Blue | 0b0001111100000000 | 0xFF0000 |
// +-------+--------------------+----------+

// George: coordinates to stay within valid image boundaries.
static inline int clamp_val(int val, int min_val, int max_val) {
    return DL_MAX(min_val, DL_MIN(val, max_val));
}

// George: Modified version for RGB565 (uint16_t) image buffers.
void draw_detection_result(uint16_t* image_ptr, int image_height, int image_width, std::list<dl::detect::result_t>& results)
{
    int i = 0;
    // George: Iterate through each detected face result.
    for (std::list<dl::detect::result_t>::iterator prediction = results.begin(); prediction != results.end(); prediction++, i++)
    {
        // George: Assuming prediction->box[] contains [x_min, y_min, x_max, y_max]
        // all bounding box coordinates within the image dimensions.
        int x_min = clamp_val(prediction->box[0], 0, image_width - 1);
        int y_min = clamp_val(prediction->box[1], 0, image_height - 1);
        int x_max = clamp_val(prediction->box[2], 0, image_width - 1);
        int y_max = clamp_val(prediction->box[3], 0, image_height - 1);

        // George: Optional: Handle cases where clamped coordinates might be invalid (e.g., x_min > x_max).
        // The drawing function should handle this, defensive checks can be added.
        // if (x_min > x_max) std::swap(x_min, x_max); 
        // if (y_min > y_max) std::swap(y_min, y_max);

        // George: Only draw if the clamped box is valid and has some area.
        if (x_min < image_width && y_min < image_height && x_max >= 0 && y_max >= 0 && x_min <= x_max && y_min <= y_max) {
            dl::image::draw_hollow_rectangle(image_ptr, image_height, image_width,
                x_min,
                y_min,
                x_max,
                y_max,
                0b1110000000000111); // George: Green color for bounding box
        }

        if (prediction->keypoint.size() == 10) 
        {
            int x_le = clamp_val(prediction->keypoint[0], 0, image_width - 1);
            int y_le = clamp_val(prediction->keypoint[1], 0, image_height - 1);
            dl::image::draw_point(image_ptr, image_height, image_width, x_le, y_le, 4, 0b0000000011111000); // George: Left eye (Red)

            int x_mlc = clamp_val(prediction->keypoint[2], 0, image_width - 1);
            int y_mlc = clamp_val(prediction->keypoint[3], 0, image_height - 1);
            dl::image::draw_point(image_ptr, image_height, image_width, x_mlc, y_mlc, 4, 0b0000000011111000); // George: Mouth left corner (Red)

            int x_n = clamp_val(prediction->keypoint[4], 0, image_width - 1);
            int y_n = clamp_val(prediction->keypoint[5], 0, image_height - 1);
            dl::image::draw_point(image_ptr, image_height, image_width, x_n, y_n, 4, 0b1110000000000111); // George: Nose (Green)

            int x_re = clamp_val(prediction->keypoint[6], 0, image_width - 1);
            int y_re = clamp_val(prediction->keypoint[7], 0, image_height - 1);
            dl::image::draw_point(image_ptr, image_height, image_width, x_re, y_re, 4, 0b0001111100000000); // George: Right eye (Blue)

            int x_mrc = clamp_val(prediction->keypoint[8], 0, image_width - 1);
            int y_mrc = clamp_val(prediction->keypoint[9], 0, image_height - 1);
            dl::image::draw_point(image_ptr, image_height, image_width, x_mrc, y_mrc, 4, 0b0001111100000000); // George: Mouth right corner (Blue)
        }
    }
}

// George: This is the overloaded version for RGB8