#include <dirent.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/sdmmc_host.h"

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

void app_main(void)
{
    ESP_LOGI(TAG, "App start");

    display_t display = { 0 };

    ESP_ERROR_CHECK(display_init(&display));

    esp_err_t sd_ret = sdcard_demo_sdspi();
    if (sd_ret != ESP_OK) {
        ESP_LOGE(TAG, "sdcard_demo_sdspi failed: 0x%x", sd_ret);
    }

    ui_init(&display);

    while (true) {
        if (display.touch && esp_lcd_touch_read_data(display.touch) == ESP_OK) {
            esp_lcd_touch_point_data_t points[1];
            uint8_t point_count = 0;

            if (esp_lcd_touch_get_data(display.touch, points, &point_count, 1) == ESP_OK && point_count > 0) {
                uint16_t x = points[0].x;
                uint16_t y = points[0].y;
                uint16_t strength = points[0].strength;

                ESP_LOGI(TAG, "Touch: x=%u y=%u strength=%u", x, y, strength);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
