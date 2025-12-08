# ESP32-035  - Chinese Yellow Display -- Satelite Tracker.

This project consist in creating something similar to G-Predict for this ESP32-035, which is a CYD MCU Screen combo. 

Status of the proyect and ongoing development is on the todo.md

idf.py is used for build/debug of this project.

HW explanation comes from this reference project.

CYD devices are little hackier and several version from several suppliers that try to cut corners. GPIOS/Other stuff will work from the ground up. 
To check if your board is compatible with this project. The first boilerplate/demo with Touch Display is stored in a branch. -- If not, GPIOs are easily changed in board_pins.h. The demo is a brief modification of the test.c files that lvgl provides.

Based on the reference project: [text](https://github.com/littleCdev/ESP32-035)
CYD Bible : [text](https://github.com/witnessmenow/ESP32-Cheap-Yellow-Display)

SD card example created from based from : [text](https://randomnerdtutorials.com/esp32-microsd-card-arduino)

Image taken from [text](https://visibleearth.nasa.gov/images/73726/june-blue-marble-next-generation-w-topography-and-bathymetry)

## Build & flash (ESP-IDF v5.1.6) // Also to be used with ESP-IDF VSCode Extension
requisites : esp-idf 5.1.6 & build-essentials

```bash
source ~/esp/esp-idf/export.sh
cd /home/dgonzalez/tft-satelite-tracker
idf.py -p /dev/ttyUSB0 build flash monitor
```
Adjust `/dev/ttyUSB0` to your serial port. (Device USB is called CH340)