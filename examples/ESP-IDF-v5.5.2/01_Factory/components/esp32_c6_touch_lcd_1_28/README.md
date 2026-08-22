# BSP: Waveshare ESP32-C6-Touch-LCD-1.28

Board Support Package for the ESP32-C6-Touch-LCD-1.28 board.

Use `#include "bsp/esp-bsp.h"` or `#include "bsp/esp32_c6_touch_lcd_1_28.h"`.

## Hardware

- MCU: ESP32-C6
- LCD: 1.28-inch 240 x 240 SPI GC9A01
- Touch: CST816S over I2C
- Storage: microSD over the same SPI bus as the LCD

This BSP does not use QMI8658, PCF85063A, CH32 IO expander, audio, SHTC3, or PSRAM.
