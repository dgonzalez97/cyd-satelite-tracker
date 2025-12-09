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
    ESP_LOGI(TAG, "Satellite created from TLE. Epoch %04d-%02d-%02d %02d:%02d:%06.3f", 
        epoch_dt.year, epoch_dt.month, epoch_dt.day, epoch_dt.hour, epoch_dt.min, epoch_dt.sec);

    return ESP_OK;
}

void orbit_sat_destroy(orbit_sat_t *sat) {
    if (!sat) {
        return;
    }
    ESP_LOGI(TAG, "Destroy satellite handle");
    delete sat;
}

// Unix UTC seconds -> JulianDate
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
    double delta_days = t - sat->sat.epoch();

    ESP_LOGI(TAG, "Propagate unix=%lld, delta_days=%.6f", (long long)unix_time_sec, delta_days);

    StateVector sv;
    Sgp4Error err = sat->sat.propagate(t, sv);
    if (err != Sgp4Error::NONE) {
        ESP_LOGE(TAG, "propagate failed (err=%d)", (int)err);
        return ESP_FAIL;
    }

    out_eci->x = sv.position[0];
    out_eci->y = sv.position[1];
    out_eci->z = sv.position[2];

    ESP_LOGI(TAG, "ECI [km] x=%.3f y=%.3f z=%.3f", out_eci->x, out_eci->y, out_eci->z);

    return ESP_OK;
}
} 