#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

typedef struct {
    esp_lcd_panel_io_handle_t io;    // SPI IO handle for ST7796
    esp_lcd_panel_handle_t    panel;
    esp_lcd_touch_handle_t    touch;
    esp_lcd_panel_io_handle_t touch_io; 
} display_t;
 
// Initialize SPI bus, LCD panel and touch.
esp_err_t display_init(display_t *disp);

void fill_screen(display_t *disp, uint16_t color);
