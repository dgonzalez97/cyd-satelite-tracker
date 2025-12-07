#pragma once

#include "driver/spi_master.h"

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
#define MOUNT_POINT "/sdcard"   
// SDMMC pins 
#define SD_PIN_CLK   18
#define SD_PIN_MOSI  23
#define SD_PIN_MISO  19
#define SD_PIN_CS    5
#define SD_HOST      SPI3_HOST