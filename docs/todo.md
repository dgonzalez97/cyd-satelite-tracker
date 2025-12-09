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


### Milestone 2: GUI with LVGL

- [x] Add `esp_lvgl_port` component and build LVGL.
- [x] Initialize LVGL after the display driver.
- [x] Display a world-map image full-screen as LVGL image (background).
- [x] Add a simple LVGL UI on top (satellite dot + touch logging on map).
- [ ] Add labels / placeholders for satellite name and basic data.
- [ ] Add a small button or text overlay to validate touch mapping (e.g. show coords).
- [ ] Document LVGL configuration in README (how to enable PNG/JPEG, etc.).
- [x] Commit: `feat: integrate LVGL and display map UI (background + placeholders)`.

### Milestone 3: TLE Parsing & SGP4 Orbit Propagation

- [x] Add `perturb` SGP4 library as git submodule under `external/perturb`.
- [x] Wire `perturb` with CMake and link into the `main` component.
- [x] Implement `main/orbit/orbit_perturb.cpp` C++ shim using `perturb::Satellite`.
- [x] Hardcode LUR-1 TLE (NORAD 60506) from CelesTrak and create a satellite from it.
- [x] Propagate LUR-1 to fixed Unix time (UTC 2025-12-09 23:00) and log ECI-compare with gpredict and make sure its okey.
- [ ] Add helper to load TLEs from internet.
- [ ] Support multiple satellites using the same C API. Rotate them touching the screen (buttons?).
- [ ] Replace fixed time with real SNTP/RTC time (Unix seconds/ESP32 time library/when connecting to the internet.).
- [ ] Commit: `feat: add perturb-based SGP4 propagation for satellites`.

### Milestone 4: Plot Satellites on Map

- [ ] Implement conversion from ECI (TEME) output to lat/lon (WGS84).
- [ ] Map lat/lon to pixel coordinates on the LVGL world map.
- [ ] Replace the current animated dot with a real-position marker for LUR-1.
- [ ] Add LVGL markers (circles/icons) for up to 3 satellites with different colors/buttons to switc.
- [ ] Periodically update satellite positions (e.g. every 60 s) and move markers.
- [ ] Show simple text with satellite name and maybe next AOS/LOS.
- [ ] Commit: `feat: display live satellite positions on map`.