#include "esp_log.h"
#include "orbit.h"

#include <ctime>
#include <string>

#include <perturb/perturb.hpp>

static const char *TAG = "orbit";

using perturb::DateTime;
using perturb::JulianDate;
using perturb::Satellite;
using perturb::Sgp4Error;
using perturb::StateVector;

struct orbit_sat_t {
    Satellite sat;
};

// LUR-1 TLE (NORAD 60506) from CelesTrak
const char *ORBIT_TLE_LUR1_L1 = "1 60506U 24149AQ  25342.16685245  .00010984  00000+0  41796-3 0  9991";
const char *ORBIT_TLE_LUR1_L2 = "2 60506  97.3940  57.6190 0002917 258.9691 101.1220 15.26924079 72795";

extern "C" {

esp_err_t orbit_sat_create_from_tle(const char *tle_line1, const char *tle_line2, orbit_sat_t **out_sat) {
    if (!tle_line1 || !tle_line2 || !out_sat) {
        ESP_LOGE(TAG, "orbit_sat_create_from_tle: invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    std::string l1(tle_line1);
    std::string l2(tle_line2);

    Satellite sat = Satellite::from_tle(l1, l2);
    if (sat.last_error() != Sgp4Error::NONE) {
        ESP_LOGE(TAG, "Satellite::from_tle failed (err=%d)", (int)sat.last_error());
        return ESP_FAIL;
    }

    orbit_sat_t *handle = new orbit_sat_t{sat};
    if (!handle) {
        ESP_LOGE(TAG, "orbit_sat_create_from_tle: no mem");
        return ESP_ERR_NO_MEM;
    }

    *out_sat = handle;

    auto epoch_dt = sat.epoch().to_datetime();
    ESP_LOGI(TAG, "Satellite created. Epoch %04d-%02d-%02d %02d:%02d:%06.3f", epoch_dt.year, epoch_dt.month, epoch_dt.day, epoch_dt.hour, epoch_dt.min, epoch_dt.sec);

    return ESP_OK;
}

void orbit_sat_destroy(orbit_sat_t *sat) {
    if (!sat) return;
    
    ESP_LOGI(TAG, "Destroy satellite handle");
    delete sat;
}

static JulianDate unix_to_julian(int64_t unix_time_sec) {
    time_t t = (time_t)unix_time_sec;
    struct tm tm_utc;
    gmtime_r(&t, &tm_utc);

    DateTime dt;
    dt.year = tm_utc.tm_year + 1900;
    dt.month = tm_utc.tm_mon + 1;
    dt.day = tm_utc.tm_mday;
    dt.hour = tm_utc.tm_hour;
    dt.min = tm_utc.tm_min;
    dt.sec = (double)tm_utc.tm_sec;

    return JulianDate(dt);
}

esp_err_t orbit_sat_propagate_unix(orbit_sat_t *sat, int64_t unix_time_sec, orbit_eci_t *out_eci) {
    if (!sat || !out_eci) {
        ESP_LOGE(TAG, "orbit_sat_propagate_unix: invalid args");
        return ESP_ERR_INVALID_ARG;
    }

    JulianDate t = unix_to_julian(unix_time_sec);

    StateVector sv;
    Sgp4Error err = sat->sat.propagate(t, sv);
    if (err != Sgp4Error::NONE) {
        ESP_LOGE(TAG, "propagate failed (err=%d)", (int)err);
        return ESP_FAIL;
    }

    out_eci->x = sv.position[0];
    out_eci->y = sv.position[1];
    out_eci->z = sv.position[2];

    return ESP_OK;
}

// multi-sat

esp_err_t orbit_system_init(orbit_system_t *sys) {
    if (!sys)
        return ESP_ERR_INVALID_ARG;
    for (int i = 0; i < ORBIT_MAX_SATS; ++i) {
        sys->sats[i] = nullptr;
    }
    sys->count = 0;
    return ESP_OK;
}

esp_err_t orbit_system_add_sat(orbit_system_t *sys, orbit_sat_t *sat, int *out_index) {
    if (!sys || !sat)
        return ESP_ERR_INVALID_ARG;

    for (int i = 0; i < ORBIT_MAX_SATS; ++i) {
        if (sys->sats[i] == nullptr) {
            sys->sats[i] = sat;
            if (sys->count < (uint8_t)(i + 1)) {
                sys->count = (uint8_t)(i + 1);
            }
            if (out_index)
                *out_index = i;
            ESP_LOGI(TAG, "orbit_system_add_sat: index %d", i);
            return ESP_OK;
        }
    }

    ESP_LOGE(TAG, "orbit_system_add_sat: no free slots");
    return ESP_ERR_NO_MEM;
}

orbit_sat_t *orbit_system_get_sat(const orbit_system_t *sys, int index) {
    if (!sys || index < 0 || index >= ORBIT_MAX_SATS)
        return nullptr;
    return sys->sats[index];
}

void orbit_system_clear(orbit_system_t *sys, bool destroy_sats) {
    if (!sys)
        return;
    for (int i = 0; i < ORBIT_MAX_SATS; ++i) {
        if (destroy_sats && sys->sats[i]) {
            orbit_sat_destroy(sys->sats[i]);
        }
        sys->sats[i] = nullptr;
    }
    sys->count = 0;
}

esp_err_t orbit_system_propagate_unix(const orbit_system_t *sys, int64_t unix_time_sec, orbit_eci_t *out_array, int out_array_len) {
    if (!sys || !out_array)
        return ESP_ERR_INVALID_ARG;

    int n = (int)sys->count;
    if (out_array_len < n)
        return ESP_ERR_INVALID_SIZE;

    for (int i = 0; i < n; ++i) {
        if (sys->sats[i]) {
            esp_err_t err = orbit_sat_propagate_unix(sys->sats[i], unix_time_sec, &out_array[i]);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "orbit_system_propagate_unix: sat %d failed (0x%x)", i, err);
                return err;
            }
        } else {
            out_array[i].x = out_array[i].y = out_array[i].z = 0.0;
        }
    }

    return ESP_OK;
}

} // extern "C"
