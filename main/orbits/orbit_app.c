#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include <stdint.h>
#include <time.h>

#include "esp_err.h"
#include "esp_log.h"

#include "board_pins.h"
#include "map_projection.h"
#include "orbit.h"
#include "orbit_geo.h"
#include "ui.h"

static const char *TAG = "orbit_app";

static orbit_system_t s_orbit_system;

static void orbit_task(void *arg) {
    (void)arg;

    ESP_LOGI(TAG, "orbit_task started");

    orbit_eci_t eci[ORBIT_MAX_SATS];

    while (1) {
        // UTC 2025-12-09 23:00:00
        int64_t now_unix = 1765321200;

        esp_err_t err = orbit_system_propagate_unix(&s_orbit_system, now_unix, eci, ORBIT_MAX_SATS);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "orbit_system_propagate_unix failed: 0x%x", err);
            vTaskDelay(pdMS_TO_TICKS(1000));
            continue;
        }

        for (int i = 0; i < s_orbit_system.count && i < ORBIT_MAX_SATS && i < UI_MAX_SATS; ++i) {
            if (!orbit_system_get_sat(&s_orbit_system, i))
                continue;

            double lat_deg = 0.0;
            double lon_deg = 0.0;
            double alt_km = 0.0;

            err = orbit_eci_to_geodetic(&eci[i], now_unix, &lat_deg, &lon_deg, &alt_km);
            if (err != ESP_OK) {
                ESP_LOGE(TAG, "orbit_eci_to_geodetic failed for sat %d: 0x%x", i, err);
                continue;
            }

            int x_px = 0;
            int y_px = 0;
            map_project_equirect(lat_deg, lon_deg, LCD_H_RES, LCD_V_RES, &x_px, &y_px);

            ui_set_satellite_pixel(i, x_px, y_px);
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void orbit_app_start(void) {
    ESP_LOGI(TAG, "orbit_app_start");

    ESP_ERROR_CHECK(orbit_system_init(&s_orbit_system));

    orbit_sat_t *lur1 = NULL;
    ESP_LOGI(TAG, "Creating LUR-1 satellite");
    ESP_ERROR_CHECK(orbit_sat_create_from_tle(ORBIT_TLE_LUR1_L1, ORBIT_TLE_LUR1_L2, &lur1));

    int index = -1;
    ESP_ERROR_CHECK(orbit_system_add_sat(&s_orbit_system, lur1, &index));
    ESP_LOGI(TAG, "LUR-1 at index %d", index);

    const uint32_t stack_words = 4096;
    BaseType_t res = xTaskCreate(orbit_task, "orbit_task", stack_words, NULL, 5, NULL);
    if (res != pdPASS) {
        ESP_LOGE(TAG, "Failed to create orbit_task");
    }
}
