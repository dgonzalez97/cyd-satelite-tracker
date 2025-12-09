#include <dirent.h>
#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/sdmmc_host.h"
#include "driver/spi_master.h"

#include "esp_log.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"

#include "sdmmc_cmd.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_xpt2046.h"

#include "board_pins.h"
#include "display.h"
#include "orbit.h"
#include "sdcard.h"
#include "ui.h"

static const char *TAG = "main";

void app_main(void) {
    ESP_LOGI(TAG, "App start");

    bool touch_debug = false;
    display_t display = (display_t){0};

    ESP_ERROR_CHECK(display_init(&display));

    esp_err_t sd_ret = sdcard_demo_sdspi();
    if (sd_ret != ESP_OK) {
        ESP_LOGE(TAG, "sdcard_demo_sdspi failed: 0x%x", sd_ret);
    }

    ESP_ERROR_CHECK(display_lvgl_init(&display));
    ui_init();

    orbit_sat_t *lur1 = NULL;
    ESP_LOGI(TAG, "Creating LUR-1 satellite from TLE");
    ESP_ERROR_CHECK(orbit_sat_create_from_tle(ORBIT_TLE_LUR1_L1, ORBIT_TLE_LUR1_L2, &lur1));

    // UTC 2025-12-09 23:00:00
    int64_t now_unix = 1765321200;
    ESP_LOGI(TAG, "Using now_unix=%lld (UTC 2025-12-09 23:00:00)", (long long)now_unix);

    orbit_eci_t lur1_eci = {0};
    esp_err_t orbit_ret = orbit_sat_propagate_unix(lur1, now_unix, &lur1_eci);
    if (orbit_ret == ESP_OK) {
        ESP_LOGI(TAG, "LUR-1 ECI [km]: x=%.3f y=%.3f z=%.3f", lur1_eci.x, lur1_eci.y, lur1_eci.z);
    } else {
        ESP_LOGE(TAG, "orbit_sat_propagate_unix failed: 0x%x", orbit_ret);
    }

    while (true) {
        uint16_t x = 0, y = 0, strength = 0;
        if (touch_debug && display_poll_touch(&display, &x, &y, &strength)) {
            ESP_LOGI(TAG, "Touch: x=%u y=%u strength=%u", x, y, strength);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
