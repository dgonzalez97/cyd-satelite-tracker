#pragma once

#include "esp_err.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

// Initialize SPI bus, LCD panel and touch.
esp_err_t display_init(esp_lcd_panel_handle_t *out_panel, esp_lcd_touch_handle_t *out_touch);

void fill_screen(esp_lcd_panel_handle_t panel, uint16_t color);
