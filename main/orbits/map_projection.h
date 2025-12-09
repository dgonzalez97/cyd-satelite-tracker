#pragma once

#include <stdint.h>

// Projects WGS-84 lat/lon to map pixels
// map_width/height are the dimensions of the rendered map in pixels.
void map_project_equirect(double lat_deg, double lon_deg, int map_width, int map_height, int *x_px, int *y_px);
