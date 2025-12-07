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
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"  //VFS https://docs.espressif.com/projects/esp-idf/en/stable/esp32/api-reference/storage/vfs.html

#include "sdmmc_cmd.h"

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_st7796.h"
#include "esp_lcd_touch.h"
#include "esp_lcd_touch_xpt2046.h"


// ESP32-035 (littleCdev) pinout, matches Hardware.md in https://github.com/littleCdev/ESP32-035
// TFT ST7796 over SPI
#define PIN_NUM_MOSI    13
#define PIN_NUM_MISO    12
#define PIN_NUM_CLK     14
#define PIN_NUM_CS      15
#define LCD_HOST        SPI2_HOST

#define PIN_NUM_DC       2
#define PIN_NUM_RST      3
#define PIN_NUM_BCKL     27

// Resistive touch (XPT2046) shares SPI bus
#define PIN_NUM_TOUCH_CS    33
#define PIN_NUM_TOUCH_IRQ   36

#define LCD_H_RES 320  // 3.5 inch ST7796 resolution 
#define LCD_V_RES 480

//SDCard 
#define MOUNT_POINT "/sdcard"   // NEW: where we mount the filesystem
// SDMMC pins 
#define SD_PIN_CLK   18
#define SD_PIN_MOSI  23
#define SD_PIN_MISO  19
#define SD_PIN_CS    5
#define SD_HOST      SPI3_HOST

static const char *TAG = "lcd_sd_demo";

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


static void sdcard_demo_sdspi(void)
{
    ESP_LOGI(TAG, "SDSPI demo: using VSPI and TF card pins");

    esp_err_t ret;

    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = false,   // assumes FAT32, don't auto-format
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_card_t *card;

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_HOST;
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;  // safe default

    spi_bus_config_t bus_cfg = {
        .mosi_io_num = SD_PIN_MOSI,
        .miso_io_num = SD_PIN_MISO,
        .sclk_io_num = SD_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4 * 512,   // 4 sectors - 
    };

    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "spi_bus_initialize failed: %s", esp_err_to_name(ret));
        return;
    }

    sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot_config.gpio_cs = SD_PIN_CS;
    slot_config.host_id = host.slot;

    ESP_LOGI(TAG, "Mounting filesystem at %s", MOUNT_POINT);
    ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);
    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. Is the card FAT32?");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card: %s", esp_err_to_name(ret));
        }
        spi_bus_free(host.slot);
        return;
    }

    ESP_LOGI(TAG, "Filesystem mounted");

    sdmmc_card_print_info(stdout, card);
    

    const char *file_path = MOUNT_POINT "/DEMOTEST.txt";
    ESP_LOGI(TAG, "Creating test file: %s", file_path);
    
    FILE *f = fopen(file_path, "w");
    if (!f) {
        ESP_LOGE(TAG, " Failed to open file for writing errno=%d", errno);
    } else {
        fprintf(f, "Open SD Card FS\n");
        fclose(f);
        ESP_LOGI(TAG, "Test file written");
    }
    
    ESP_LOGI(TAG, "Listing files in %s", MOUNT_POINT);
    
    errno = 0;
    DIR *dir = opendir(MOUNT_POINT);
    if (!dir) {
        ESP_LOGE(TAG, "opendir(%s) failed, errno=%d (%s)", MOUNT_POINT, errno, strerror(errno));
        return;
    }
    
    errno = 0;
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        ESP_LOGI(TAG, " %s", entry->d_name);
    }
    
    if (errno != 0) {
        ESP_LOGE(TAG, "readdir(%s) failed, errno=%d (%s)",MOUNT_POINT, errno, strerror(errno));
    }

    if (closedir(dir) != 0) {
        ESP_LOGW(TAG, "closedir(%s) failed, errno=%d (%s)", MOUNT_POINT, errno, strerror(errno));
    
    }
        // Optional cleanup, not needed for demo.
    // esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
    // spi_bus_free(host.slot);
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
    ESP_ERROR_CHECK(spi_bus_initialize(LCD_HOST, &buscfg, 0));  // No DMA because it colapses with the DMA initialization in sdspi demo

    esp_lcd_panel_io_handle_t io_handle = NULL;
    esp_lcd_panel_io_spi_config_t io_config = {  // Extracted from esp_lcd_st7796.h example
        .dc_gpio_num = PIN_NUM_DC,
        .cs_gpio_num = PIN_NUM_CS,
        .pclk_hz = 20 * 1000 * 1000, // 20 MHZ, but top is 40MHZ for ST7796
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
        .rgb_endian = LCD_RGB_ENDIAN_RGB,  // depends on wiring. Adjust if colors arent red and white
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(io_handle, &panel_config, &panel_handle));

    ESP_LOGI(TAG, "Reset and init panel");
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel_handle, true, false)); // adjust to rotate/mirror if needed
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, false)); // set true if colors look inverted /Red and white
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    enable_backlight();

    ESP_LOGI(TAG, "Filling screen with test colors");
    fill_screen(panel_handle, 0xF800); // Red
    vTaskDelay(pdMS_TO_TICKS(300));
    fill_screen(panel_handle, 0xFFFF); // White
    vTaskDelay(pdMS_TO_TICKS(300));

    // XPT2046 touch via esp_lcd_touch driver on shared SPI bus (bitbanging from CYD examples)
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
            .mirror_y = true,  // 
        },
        .interrupt_callback = NULL,
    };

    esp_err_t tp_ret = esp_lcd_touch_new_spi_xpt2046(tp_io, &tp_cfg, &touch_handle);
    if (tp_ret != ESP_OK) {
        ESP_LOGE(TAG, "XPT2046 touch init failed (err=0x%x). Diferent CYD may be the reason .", tp_ret);
    } else {
        ESP_LOGI(TAG, "XPT2046 touch driver initialized");
    }

    sdcard_demo_sdspi();

    while (true)
    {
        if (touch_handle && esp_lcd_touch_read_data(touch_handle) == ESP_OK){
            esp_lcd_touch_point_data_t points[1];
            uint8_t point_count = 0;

            if (esp_lcd_touch_get_data(touch_handle, points, &point_count, 1) == ESP_OK && point_count > 0){
                uint16_t x = points[0].x;
                uint16_t y = points[0].y;
                uint16_t strength = points[0].strength;

                ESP_LOGI(TAG, "Touch: x=%u y=%u strength=%u", x, y, strength);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
