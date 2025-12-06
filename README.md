# ESP32-035 ST7796 + XPT2046 Demo (ESP-IDF)

Minimal bring-up for the ESP32-035 (ESP32-WROOM-32, orange PCB) with a 3.5" ST7796 SPI display and XPT2046 resistive touch on the same SPI bus. Based on the reference project: https://github.com/littleCdev/ESP32-035

## Hardware (matches Hardware.md from the reference repo)
- TFT ST7796 (SPI): MOSI13, MISO12, SCLK14, CS15, DC2, RST3, BKLT27 (backlight active high).
- Touch XPT2046 (resistive): CS33, IRQ36, shares MOSI/MISO/SCLK above.
- Resolution: 320x480.

## What it does
- Initializes the ST7796 panel, enables the backlight, and fills test colors.
- Initializes XPT2046 via `esp_lcd_touch` and logs touch coordinates over UART.

## Build & flash (WSL2 + ESP-IDF v5.1.x)
```bash
source ~/esp/esp-idf/export.sh
cd /home/dgonzalez/tft-satelite-tracker
idf.py -p /dev/ttyUSB0 build flash monitor
```
Adjust `/dev/ttyUSB0` to your serial port.

## Pin/Orientation tweaks
- Adjust pins at the top of `main/main.c` if your wiring differs.
- Flip orientation with `esp_lcd_panel_mirror` or `esp_lcd_panel_invert_color`.
- Touch axes/flip: update `swap_xy`, `mirror_x`, `mirror_y` in the XPT2046 touch config.

## Notes
- SPI clock starts at 20 MHz for panel; you can raise toward 40 MHz once stable.
- Backlight is on GPIO27, active high.
- Touch uses the `atanisoft/esp_lcd_touch_xpt2046` component via the ESP-IDF component manager.
