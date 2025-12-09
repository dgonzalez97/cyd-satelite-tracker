#include "orbit_geo.h"
#include <math.h>

#define WGS84_A_KM 6378.137
#define WGS84_F (1.0 / 298.257223563)
#define WGS84_E2 (2.0 * WGS84_F - WGS84_F * WGS84_F)

static double unix_to_jd(int64_t unix_time_sec) { return ((double)unix_time_sec) / 86400.0 + 2440587.5; }

static double gmst_from_unix(int64_t unix_time_sec) {
    double jd = unix_to_jd(unix_time_sec);
    double t = (jd - 2451545.0) / 36525.0;

    double gmst_deg = 280.46061837 + 360.98564736629 * (jd - 2451545.0) + 0.000387933 * t * t - (t * t * t) / 38710000.0;

    gmst_deg = fmod(gmst_deg, 360.0);
    if (gmst_deg < 0.0)
        gmst_deg += 360.0;

    return gmst_deg * (M_PI / 180.0);
}

esp_err_t orbit_eci_to_geodetic(const orbit_eci_t *eci, int64_t unix_time_sec, double *lat_deg, double *lon_deg, double *alt_km) {
    if (!eci || !lat_deg || !lon_deg || !alt_km)
        return ESP_ERR_INVALID_ARG;

    double theta = gmst_from_unix(unix_time_sec);
    double c = cos(theta);
    double s = sin(theta);

    double x_eci = eci->x;
    double y_eci = eci->y;
    double z_eci = eci->z;

    double x = c * x_eci + s * y_eci;
    double y = -s * x_eci + c * y_eci;
    double z = z_eci;

    double a = WGS84_A_KM;
    double e2 = WGS84_E2;

    double r = sqrt(x * x + y * y);

    double lon = atan2(y, x);
    double lat = atan2(z, r * (1.0 - e2));
    double alt = 0.0;

    for (int i = 0; i < 5; ++i) {
        double sin_lat = sin(lat);
        double N = a / sqrt(1.0 - e2 * sin_lat * sin_lat);
        alt = r / cos(lat) - N;
        lat = atan2(z, r * (1.0 - e2 * (N / (N + alt))));
    }

    *lat_deg = lat * (180.0 / M_PI);
    *lon_deg = lon * (180.0 / M_PI);
    *alt_km = alt;

    return ESP_OK;
}
