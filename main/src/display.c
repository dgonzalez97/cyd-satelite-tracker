#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "driver/gpio.h"
#include "driver/spi_master.h"

#include "esp_check.h"
#include "esp_log.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_xpt2046.h"

#include "board_pins.h"
#include "display.h"

static const char *TAG = "display_demo";

static void enable_backlight(void) {
    gpio_config_t bklt_config = {
        .pin_bit_mask = 1ULL << PIN_NUM_BCKL,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bklt_config);
    gpio_set_level(PIN_NUM_BCKL, 1);
}

void fill_screen(esp_lcd_panel_handle_t panel, uint16_t color) {
    static uint16_t line_buf[LCD_H_RES];
    for (int x = 0; x < LCD_H_RES; x++) {
        line_buf[x] = color;
    }

    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y + 1, line_buf);
    }
}

esp_err_t display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch) {
    ESP_RETURN_ON_FALSE(out_panel, ESP_ERR_INVALID_ARG, TAG, "out_panel is NULL");
    ESP_RETURN_ON_FALSE(out_touch, ESP_ERR_INVALID_ARG, TAG, "out_touch is NULL");

    ESP_LOGI(TAG, "Initializing SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 60 * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(LCD_HOST, &buscfg, 0), TAG, "spi_bus_initialize failed");

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        // Extracted from esp_lcd_st7796.h example
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000, // 20 MHZ, but top is 40MHZ for ST7796
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .flags =
            {
                .dc_low_on_data = false,
            },
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle), TAG, "esp_lcd_new_panel_io_spi failed");

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB, // depends on wiring. Adjust if colors arent red and white
        .bits_per_pixel = 16,
    };
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle), TAG, "esp_lcd_new_panel_st7796 failed");

    ESP_LOGI(TAG, "Reset and init panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "panel init failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle, true, false), TAG, "panel mirror failed"); // adjust to rotate/mirror if needed
    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, false), TAG, "invert color failed"); // set true if colors look inverted /Red and white
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "disp_on_off failed");
    enable_backlight();

    ESP_LOGI(TAG, "Filling screen with test colors");
    fill_screen(panel_handle, 0xF800); // Red
    vTaskDelay(pdMS_TO_TICKS(300));
    fill_screen(panel_handle, 0xFFFF); // White
    vTaskDelay(pdMS_TO_TICKS(300));

    // XPT2046 touch via esp_lcd_touch driver on shared SPI bus (bitbanging from CYD examples)
    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_TOUCH_CS);
    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &tp_io_cfg, &tp_io), TAG, "esp_lcd_new_panel_io_spi for touch failed");

    esp_lcd_touch_handle_t touch_handle = NULL;
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = PIN_NUM_TOUCH_IRQ,
        .levels =
            {
                .reset = 0,
                .interrupt = 0, // PENIRQ active low
            },
        .flags =
            {
                .swap_xy = true, // common for this board; flip if logs are rotated
                .mirror_x = false,
                .mirror_y = true, //
            },
        .interrupt_callback = NULL,
    };

    esp_err_t tp_ret = esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &touch_handle);
    if (tp_ret != ESP_OK) {
        ESP_LOGE(TAG, "XPT2046 touch init failed (err=0x%x). Diferent CYD may be the reason .", tp_ret);
        // not fatal; touch_handle stays NULL
    } else {
        ESP_LOGI(TAG, "XPT2046 touch driver initialized");
    }

    *out_panel = panel_handle;
    *out_touch = touch_handle;

    return ESP_OK;
}
