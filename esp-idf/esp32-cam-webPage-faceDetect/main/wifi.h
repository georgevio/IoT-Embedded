#ifndef WIFI_H
#define WIFI_H

#include "esp_err.h"

#ifdef __cplusplus  // If this is compiled by a C++ compiler
extern "C" {        // Tell the C++ compiler that the functions have C linkage
#endif

/**
 * @brief Initialize WiFi in Station mode.
 */
void wifi_init_sta(void);

#ifdef __cplusplus
} // End of extern "C"
#endif

#endif // WIFI_H