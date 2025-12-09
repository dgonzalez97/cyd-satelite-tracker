#pragma once

#include "esp_err.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct orbit_sat_t orbit_sat_t;

typedef struct {
    double x;
    double y;
    double z;
} orbit_eci_t;

// Create a satellite handle from a TLE
esp_err_t orbit_sat_create_from_tle(const char *tle_line1, const char *tle_line2, orbit_sat_t **out_sat);

// Destroy satellite handle
void orbit_sat_destroy(orbit_sat_t *sat);

esp_err_t orbit_sat_propagate_unix(orbit_sat_t *sat, int64_t unix_time_sec, orbit_eci_t *out_eci);

// Hardcoded LUR-1 TLE (from CelesTrak)// On next milestones this disapears
extern const char *ORBIT_TLE_LUR1_L1;
extern const char *ORBIT_TLE_LUR1_L2;

#ifdef __cplusplus
}
#endif
