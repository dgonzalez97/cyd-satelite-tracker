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

#include "esp_lvgl_port.h"
#include "lvgl.h"

#include "board_pins.h"
#include "display.h"

static const char *TAG = "display";

// Panel-specific color constants (calibrated to what you actually see/ used for debug)
#define PANEL_COLOR_RED 0xF800
#define PANEL_COLOR_GREEN 0x001F
#define PANEL_COLOR_BLUE 0x07E0

static lv_disp_t *s_lv_disp = NULL;
static lv_indev_t *s_lv_touch = NULL;

static void enable_backlight(void) {
    gpio_config_t bklt_config = {
        .pin_bit_mask = 1ULL << PIN_NUM_BCKL,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bklt_config);
    gpio_set_level(PIN_NUM_BCKL, 1);
}

void fill_screen(display_t *disp, uint16_t color) {
    if (!disp || !disp->panel) {
        return;
    }

    static uint16_t line_buf[LCD_H_RES];
    for (int x = 0; x < LCD_H_RES; x++) {
        line_buf[x] = color;
    }

    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(disp->panel, 0, y, LCD_H_RES, y + 1, line_buf);
    }
}

#if DEBUG_DISPLAY
static void vertical_color_sweep(display_t *disp, uint16_t color, uint32_t duration_ms) {
    if (!disp || !disp->panel) {
        return;
    }

    static uint16_t col_buf[LCD_V_RES];
    for (int y = 0; y < LCD_V_RES; y++) {
        col_buf[y] = color;
    }

    uint32_t per_col_ms = duration_ms / LCD_H_RES;
    if (per_col_ms == 0) {
        per_col_ms = 1;
    }

    for (int x = 0; x < LCD_H_RES; x++) {
        esp_lcd_panel_draw_bitmap(disp->panel, x, 0, x + 1, LCD_V_RES, col_buf);
        vTaskDelay(pdMS_TO_TICKS(per_col_ms));
    }
}

static void horizontal_wipe(display_t *disp, uint16_t color, uint32_t duration_ms) {
    if (!disp || !disp->panel) {
        return;
    }

    static uint16_t row_buf[LCD_H_RES];
    for (int x = 0; x < LCD_H_RES; x++) {
        row_buf[x] = color;
    }

    uint32_t per_row_ms = duration_ms / LCD_V_RES;
    if (per_row_ms == 0) {
        per_row_ms = 1;
    }

    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(disp->panel, 0, y, LCD_H_RES, y + 1, row_buf);
        vTaskDelay(pdMS_TO_TICKS(per_row_ms));
    }
}
#endif

esp_err_t display_init(display_t *disp) {

    ESP_RETURN_ON_FALSE(disp, ESP_ERR_INVALID_ARG, TAG, "display_t pointer is NULL");

    disp->io = NULL;
    disp->panel = NULL;
    disp->touch = NULL;
    disp->touch_io = NULL;

    disp->rotation_swap_xy = true;
    disp->rotation_mirror_x = false;
    disp->rotation_mirror_y = false;

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
    esp_lcd_panel_handle_t panel_handle = NULL;

    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000,
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

    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_BGR,
        .bits_per_pixel = 16,
    };

    ESP_RETURN_ON_ERROR(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle), TAG, "esp_lcd_new_panel_st7796 failed");

    ESP_LOGI(TAG, "Reset and init panel");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_reset(panel_handle), TAG, "panel reset failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_init(panel_handle), TAG, "panel init failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_swap_xy(panel_handle, disp->rotation_swap_xy), TAG, "panel swap_xy failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_mirror(panel_handle, disp->rotation_mirror_x, disp->rotation_mirror_y), TAG, "panel mirror failed");

    ESP_RETURN_ON_ERROR(esp_lcd_panel_invert_color(panel_handle, false), TAG, "invert color failed");
    ESP_RETURN_ON_ERROR(esp_lcd_panel_disp_on_off(panel_handle, true), TAG, "disp_on_off failed");
    enable_backlight();

#if DEBUG_DISPLAY
    display_t tmp_disp = {
        .io = io_handle,
        .panel = panel_handle,
        .touch = NULL,
        .touch_io = NULL,
        .rotation_swap_xy = disp->rotation_swap_xy,
        .rotation_mirror_x = disp->rotation_mirror_x,
        .rotation_mirror_y = disp->rotation_mirror_y,
    };

    ESP_LOGI(TAG, "Running display demo: RGB sweeps and black wipe");
    vertical_color_sweep(&tmp_disp, PANEL_COLOR_RED, 2000);
    vertical_color_sweep(&tmp_disp, PANEL_COLOR_GREEN, 2000);
    vertical_color_sweep(&tmp_disp, PANEL_COLOR_BLUE, 2000);
    horizontal_wipe(&tmp_disp, 0x0000, 1000);
#endif

    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_TOUCH_CS);

    esp_err_t tp_io_ret = esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &tp_io_cfg, &tp_io);
    if (tp_io_ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_lcd_new_panel_io_spi for touch failed (err=0x%x). Continuing without touch.", tp_io_ret);
        tp_io = NULL;
    }

    esp_lcd_touch_handle_t touch_handle = NULL;

    if (tp_io != NULL) {
        esp_lcd_touch_config_t tp_cfg = {
            .x_max = LCD_H_RES,
            .y_max = LCD_V_RES,
            .rst_gpio_num = GPIO_NUM_NC,
            .int_gpio_num = PIN_NUM_TOUCH_IRQ,
            .levels =
                {
                    .reset = 0,
                    .interrupt = 0,
                },
            .flags =
                {
                    .swap_xy = disp->rotation_swap_xy,
                    .mirror_x = disp->rotation_mirror_x,
                    .mirror_y = disp->rotation_mirror_y,
                },
            .interrupt_callback = NULL,
        };

        esp_err_t tp_ret = esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &touch_handle);
        if (tp_ret != ESP_OK) {
            ESP_LOGE(TAG, "XPT2046 touch init failed (err=0x%x). Different CYD may be the reason.", tp_ret);
            esp_lcd_panel_io_del(tp_io);
            tp_io = NULL;
        } else {
            ESP_LOGI(TAG, "XPT2046 touch driver initialized");
        }
    }

    disp->io = io_handle;
    disp->panel = panel_handle;
    disp->touch = touch_handle;
    disp->touch_io = tp_io;

    return ESP_OK;
}

esp_err_t display_lvgl_init(display_t *disp) {
    const lvgl_port_cfg_t lvgl_cfg = ESP_LVGL_PORT_INIT_CONFIG();
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "lvgl_port_init failed");

    lvgl_port_display_cfg_t disp_cfg = (lvgl_port_display_cfg_t){0};

    disp_cfg.io_handle = disp->io;
    disp_cfg.panel_handle = disp->panel;
    disp_cfg.buffer_size = LCD_H_RES * 40;
    disp_cfg.double_buffer = true;
    disp_cfg.hres = LCD_H_RES;
    disp_cfg.vres = LCD_V_RES;
    disp_cfg.monochrome = false;
    disp_cfg.color_format = LV_COLOR_FORMAT_RGB565;

    disp_cfg.rotation.swap_xy = disp->rotation_swap_xy;
    disp_cfg.rotation.mirror_x = disp->rotation_mirror_x;
    disp_cfg.rotation.mirror_y = disp->rotation_mirror_y;

    disp_cfg.flags.buff_dma = true;
    disp_cfg.flags.swap_bytes = true;

    s_lv_disp = lvgl_port_add_disp(&disp_cfg);
    ESP_RETURN_ON_FALSE(s_lv_disp, ESP_FAIL, TAG, "lvgl_port_add_disp failed");

    lv_disp_set_default(s_lv_disp);

    if (disp->touch) {
        lvgl_port_touch_cfg_t touch_cfg = (lvgl_port_touch_cfg_t){0};
        touch_cfg.disp = s_lv_disp;
        touch_cfg.handle = disp->touch;

        s_lv_touch = lvgl_port_add_touch(&touch_cfg);
        ESP_RETURN_ON_FALSE(s_lv_touch, ESP_FAIL, TAG, "lvgl_port_add_touch failed");
    } else {
        ESP_LOGW(TAG, "Touch handle is NULL, LVGL will run without touch input");
    }

    return ESP_OK;
}

bool display_poll_touch(display_t *disp, uint16_t *x, uint16_t *y, uint16_t *strength) {
    if (!disp || !disp->touch) {
        return false;
    }

    esp_err_t ret = esp_lcd_touch_read_data(disp->touch);
    if (ret != ESP_OK) {
        return false;
    }

    esp_lcd_touch_point_data_t points[1] = {0};
    uint8_t point_count = 0;
    ret = esp_lcd_touch_get_data(disp->touch, points, &point_count, 1);
    if (ret != ESP_OK || point_count == 0) {
        return false;
    }

    if (x) {
        *x = points[0].x;
    }
    if (y) {
        *y = points[0].y;
    }
    if (strength) {
        *strength = points[0].strength;
    }
    return true;
}
