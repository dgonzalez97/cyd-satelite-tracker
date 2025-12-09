#pragma once

#include "esp_err.h"
#include <stdbool.h>
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

#define ORBIT_MAX_SATS 3

typedef struct {
    orbit_sat_t *sats[ORBIT_MAX_SATS];
    uint8_t count;
} orbit_system_t;

// Single satellite API
esp_err_t orbit_sat_create_from_tle(const char *tle_line1, const char *tle_line2, orbit_sat_t **out_sat);
void orbit_sat_destroy(orbit_sat_t *sat);
esp_err_t orbit_sat_propagate_unix(orbit_sat_t *sat, int64_t unix_time_sec, orbit_eci_t *out_eci);

// Hardcoded LUR-1 TLE (temporary)
extern const char *ORBIT_TLE_LUR1_L1;
extern const char *ORBIT_TLE_LUR1_L2;

// Multi-satellite helpers -
esp_err_t orbit_system_init(orbit_system_t *sys);
esp_err_t orbit_system_add_sat(orbit_system_t *sys, orbit_sat_t *sat, int *out_index);
orbit_sat_t *orbit_system_get_sat(const orbit_system_t *sys, int index);
void orbit_system_clear(orbit_system_t *sys, bool destroy_sats);
esp_err_t orbit_system_propagate_unix(const orbit_system_t *sys, int64_t unix_time_sec, orbit_eci_t *out_array, int out_array_len);

#ifdef __cplusplus
}
#endif
