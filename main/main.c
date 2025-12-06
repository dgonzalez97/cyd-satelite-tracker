#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_xpt2046.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define LCD_HOST SPI2_HOST

// ESP32-035 (littleCdev) pinout, matches Hardware.md in https://github.com/littleCdev/ESP32-035
// TFT ST7796 over SPI
#define PIN_NUM_MOSI 13
#define PIN_NUM_MISO 12
#define PIN_NUM_CLK 14
#define PIN_NUM_CS 15
#define PIN_NUM_DC 2
#define PIN_NUM_RST 3
#define PIN_NUM_BCKL 27

// Resistive touch (XPT2046) shares SPI bus; pins listed for reference
#define PIN_NUM_TOUCH_CS 33
#define PIN_NUM_TOUCH_IRQ 36

#define LCD_H_RES 320
#define LCD_V_RES 480

static const char *TAG = "lcd_demo";

static void enable_backlight(void) {
    gpio_config_t bklt_config = {
        .pin_bit_mask = 1ULL << PIN_NUM_BCKL,
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&bklt_config);
    gpio_set_level(PIN_NUM_BCKL, 1);
}

static void fill_screen(esp_lcd_panel_handle_t panel, uint16_t color) {
    static uint16_t line_buf[LCD_H_RES];
    for (int x = 0; x < LCD_H_RES; x++) {
        line_buf[x] = color;
    }

    for (int y = 0; y < LCD_V_RES; y++) {
        esp_lcd_panel_draw_bitmap(panel, 0, y, LCD_H_RES, y + 1, line_buf);
    }
}

void app_main(void) {
    ESP_LOGI(TAG, "Initializing SPI bus");
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = LCD_H_RES * 60 * sizeof(uint16_t),
    };
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO));

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000, // Start modest; raise toward 40MHz once stable
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
        .flags = {
            .dc_low_on_data = false,
        },
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &io_config, &io_handle));

    esp_lcd_panel_handle_t panel_handle = NULL;
    esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = PIN_NUM_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));

    ESP_LOGI(TAG, "Reset and init panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false)); // adjust to rotate/mirror if needed
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false)); // set true if colors look inverted
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    enable_backlight();

    ESP_LOGI(TAG, "Filling screen with test colors");
    fill_screen(panel_handle, 0xF800); // Red
    esp_rom_delay_us(300000);
    fill_screen(panel_handle, 0x07E0); // Green
    esp_rom_delay_us(300000);
    fill_screen(panel_handle, 0x001F); // Blue
    esp_rom_delay_us(300000);
    fill_screen(panel_handle, 0xFFFF); // White
    esp_rom_delay_us(300000);

    // XPT2046 touch via esp_lcd_touch driver on shared SPI bus
    esp_lcd_panel_io_handle_t tp_io = NULL;
    esp_lcd_panel_io_spi_config_t tp_io_cfg = ESP_LCD_TOUCH_IO_SPI_XPT2046_CONFIG(PIN_NUM_TOUCH_CS);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)LCD_HOST, &tp_io_cfg, &tp_io));

    esp_lcd_touch_handle_t touch_handle = NULL;
    esp_lcd_touch_config_t tp_cfg = {
        .x_max = LCD_H_RES,
        .y_max = LCD_V_RES,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = PIN_NUM_TOUCH_IRQ,
        .levels = {
            .reset = 0,
            .interrupt = 0, // PENIRQ active low
        },
        .flags = {
            .swap_xy = true,   // common for this board; flip if logs are rotated
            .mirror_x = false,
            .mirror_y = true,  // flip if Y is inverted
        },
        .interrupt_callback = NULL,
    };

    esp_err_t tp_ret = esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &touch_handle);
    if (tp_ret != ESP_OK) {
        ESP_LOGE(TAG, "XPT2046 touch init failed (err=0x%x). Check CS=%d IRQ=%d wiring.", tp_ret, PIN_NUM_TOUCH_CS, PIN_NUM_TOUCH_IRQ);
    } else {
        ESP_LOGI(TAG, "XPT2046 touch driver initialized");
    }

    while (true) {
        if (touch_handle && esp_lcd_touch_read_data(touch_handle) == ESP_OK) {
            uint16_t x[1], y[1], strength = 0;
            uint8_t count = 0;
            if (esp_lcd_touch_get_coordinates(touch_handle, x, y, &strength, &count, 1) && count > 0) {
                ESP_LOGI(TAG, "Touch: x=%u y=%u strength=%u", x[0], y[0], strength);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
