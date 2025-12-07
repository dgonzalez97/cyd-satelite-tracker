# TODOs
- [x] Bring up ST7796 display with correct ESP32-035 pinout.
- [x] Add XPT2046 touch driver on shared SPI, log coordinates.
- [ ] Tune touch orientation/calibration if logs do not match screen corners.
- [ ] Add simple UI (text overlay or button) to validate touch mapping.
- [ ] Boot status - printed in screen before app loading (+ last error linux dmesg style)
- [ ] Add CI-friendly build instructions and screenshots in README.


### Milestone: SD card (SDSPI) bring-up

- [x] Add SDSPI SD card wiring for ESP32-035 TF slot (CLK=18, MOSI=23, MISO=19, CS=5).
- [x] Initialize SD card over VSPI using `esp_vfs_fat_sdspi_mount`.
- [x] Print SD card info (`sdmmc_card_print_info`) on boot.
- [x] List files in `/sdcard` and create a test file (`DEMOTEST.TXT`).
- [ ] Review SD card usage in README (FAT32, 8.3 filenames, expected log output).
- [ ] Refactor SD card code into a small `storage_sd` module for later TLE loading/bootloader, images etc.