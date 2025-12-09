#include <dirent.h>
#include <errno.h>
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

    while (true) {
        uint16_t x = 0, y = 0, strength = 0;
        if (touch_debug && display_poll_touch(&display, &x, &y, &strength)) {
            ESP_LOGI(TAG, "Touch: x=%u y=%u strength=%u", x, y, strength);
        }
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}
