#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define UI_MAX_SATS 3

void ui_init(void);

// Set satellite position in map pixel coordinates.
void ui_set_satellite_pixel(int sat_index, int x_px, int y_px);

#ifdef __cplusplus
}
#endif
