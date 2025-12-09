#pragma once

#include "esp_err.h"
#include "orbit.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Convert ECI (TEME-like) [km] to WGS-84 geodetic - Same as Gpredict is doing
esp_err_t orbit_eci_to_geodetic(const orbit_eci_t *eci, int64_t unix_time_sec, double *lat_deg, double *lon_deg, double *alt_km);

#ifdef __cplusplus
}
#endif
