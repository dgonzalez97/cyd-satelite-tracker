#pragma once

#include <stdbool.h>

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#define DEBUG_DISPLAY 0 // set to 1 to enable RGB debug sweeps

typedef struct {
    esp_lcd_panel_io_handle_t io;
    esp_lcd_panel_handle_t panel;
    esp_lcd_touch_handle_t touch;
    esp_lcd_panel_io_handle_t touch_io;

    bool rotation_swap_xy;
    bool rotation_mirror_x;
    bool rotation_mirror_y;
} display_t;

esp_err_t display_init(display_t *disp);
esp_err_t display_lvgl_init(display_t *disp);

void fill_screen(display_t *disp, uint16_t color);

bool display_poll_touch(display_t *disp, uint16_t *x, uint16_t *y, uint16_t *strength);
