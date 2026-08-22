# ESP32-C6-Touch-LCD-1.28 Hardware Reference

> Hardware quick reference for developers and AI tools. The content is cross-checked against this repository's `hardware/schematics/ESP32-C6-Touch-LCD-1.28.pdf` schematic and the current examples / BSP pin definitions.

## 1. Board Overview

- Product: `ESP32-C6-Touch-LCD-1.28`, SKU `35450`
- Main controller: `ESP32-C6`, RISC-V, Wi-Fi 6 / BLE / 802.15.4
- Flash: 16 MB, schematic device `XM25QH128DHIQT`
- Display: 1.28-inch round touch LCD, `240 x 240`, typically RGB565
- Main onboard resources: GC9A01 LCD, CST816S touch controller, QMI8658 IMU, PCF85063ATL RTC, TF card socket, lithium battery charge/discharge, battery voltage sampling, USB, UART, SH1.0 14-pin expansion connector

## 2. Onboard Peripherals

| Module | Device / Function | Interface | Address / Parameters | GPIO / Signal |
|---|---|---|---|---|
| LCD | GC9A01 | 4-wire SPI | 240 x 240, typically RGB565, SPI2 in examples/BSP | DC=GPIO19, CS=GPIO10, SCL=GPIO11, MOSI=GPIO21, MISO=GPIO22, RST=GPIO20, BL=GPIO7 |
| Touch | CST816S | I2C | 7-bit address `0x15` | SCL=GPIO8, SDA=GPIO18, RST=GPIO23, INT=GPIO4 |
| IMU | QMI8658 6-axis sensor | I2C | 7-bit address `0x6B` | SCL=GPIO8, SDA=GPIO18, INT1=GPIO5, INT2=GPIO4 |
| RTC | PCF85063ATL | I2C | 7-bit address `0x51`, 32.768 kHz crystal | SCL=GPIO8, SDA=GPIO18, INT=GPIO6 |
| TF card | TF card socket | SPI | Shares the SPI bus with the LCD | CS=GPIO15, SCK=GPIO11, MOSI=GPIO21, MISO=GPIO22 |
| Battery sampling | BAT divided to ADC | ADC | R1=200K, R8=100K | GPIO0 / BAT_ADC |
| Charge management | ETA6098 | Power | Single-cell lithium battery charge/discharge, ISET configured by R4 | Battery connector / Battery_Charge / BC_State |
| 3.3 V power | MP1605GTF-Z | Power | Buck output for system 3.3 V | 3V3 |
| USB Type-C | ESP32-C6 native USB | USB | Download, logs | USB_N=GPIO12, USB_P=GPIO13 |
| UART0 | Default serial port | UART | Debug / expansion pads | TXD=GPIO16, RXD=GPIO17 |

## 3. Complete GPIO Assignment

Pins occupied by onboard circuits or explicitly broken out are listed in ascending GPIO order.

| GPIO | Signal Name | Connected To | Notes |
|---:|---|---|---|
| GPIO0 | BAT_ADC | Battery voltage divider sampling | Divider network R1=200K / R8=100K |
| GPIO1 | GPIO1 | Expansion connector | Listed in the schematic GPIO table |
| GPIO2 | GPIO2 | Expansion connector | P1 / GPIO_OUT |
| GPIO3 | GPIO3 | Expansion connector | P1 / GPIO_OUT |
| GPIO4 | TP_INT / IMU_INT2 | CST816S touch interrupt / QMI8658 INT2 / expansion connector | Touch and IMU_INT2 share this GPIO; avoid interrupt conflicts |
| GPIO5 | IMU_INT1 | QMI8658 INT1 / expansion connector | - |
| GPIO6 | RTC_INT | PCF85063ATL interrupt / expansion connector | RTC is optional on the schematic |
| GPIO7 | LCD_BL | LCD backlight control | Can be used for PWM brightness control |
| GPIO8 | I2C_SCL / TP_SCL / RTC_SCL / IMU_SCL / expansion connector | Shared I2C SCL for touch + IMU + RTC / expansion connector | See Section 4 for I2C devices |
| GPIO9 | BOOT / Key1 | BOOT button / expansion connector | Strapping pin; button enters download mode |
| GPIO10 | LCD_CS | GC9A01 chip select | - |
| GPIO11 | LCD_CLK / SPI_CLK | SPI clock shared by LCD and TF card | - |
| GPIO12 | USB_N | USB Type-C D- | ESP32-C6 native USB |
| GPIO13 | USB_P | USB Type-C D+ | ESP32-C6 native USB |
| GPIO15 | SD_CS | TF card chip select | Shared with the SPI bus |
| GPIO16 | TXD | UART TX pad / expansion connector | Expansion TX |
| GPIO17 | RXD | UART RX pad / expansion connector | Expansion RX |
| GPIO18 | I2C_SDA / TP_SDA / RTC_SDA / IMU_SDA | Shared I2C SDA for touch + IMU + RTC / expansion connector | See Section 4 for I2C devices |
| GPIO19 | LCD_DC | GC9A01 data/command | - |
| GPIO20 | LCD_RST | GC9A01 reset | - |
| GPIO21 | SPI_MOSI / LCD_MOSI / SD_MOSI | SPI MOSI shared by LCD and TF card | - |
| GPIO22 | SPI_MISO / LCD_MISO / SD_MISO | TF card SPI MISO / LCD MISO / expansion connector | LCD examples can leave readback unused; TF card needs this pin |
| GPIO23 | TP_RST / BC_State | CST816S touch reset / charge status signal / expansion connector | BSP uses GPIO23 as touch reset; avoid conflicts when using it externally |

## 4. Onboard I2C Devices

| Device | Component | I2C Address | Extra Pins |
|---|---|---:|---|
| Touch | CST816S | `0x15` | RST=GPIO23, INT=GPIO4 |
| IMU | QMI8658 | `0x6B` | INT1=GPIO5, INT2=GPIO4 |
| RTC | PCF85063ATL | `0x51` | INT=GPIO6 |

Shared I2C bus: `SCL=GPIO8`, `SDA=GPIO18`.

## 5. Expansion Header

| Type | Signals |
|---|---|
| Power | `3V3` / `GND` |
| I2C | `SCL(GPIO8)` / `SDA(GPIO18)` |
| UART | `TX(GPIO16)` / `RX(GPIO17)` |
| GPIO | `GPIO1` / `GPIO2` / `GPIO3` / `GPIO4` / `GPIO5` / `GPIO6` / `GPIO8` / `GPIO9` / `GPIO16` / `GPIO17` / `GPIO18` / `GPIO22` / `GPIO23` / `RESET` |

## 6. Usage Notes

- When troubleshooting peripherals in Arduino / ESP32 projects, first check that the board target is ESP32-C6, and verify the port, baud rate, `USB CDC On Boot`, and `Core Debug Level` settings.
- When connecting external I2C devices, avoid address conflicts with `0x15`, `0x6B`, and `0x51`; touch, IMU, and RTC share `GPIO8/GPIO18`.
- `GPIO4` is connected to both the CST816S touch interrupt and QMI8658 INT2. If both interrupts are enabled, handle the shared interrupt in software or avoid using them at the same time.
- `GPIO23` is used as CST816S touch reset in the current BSP, and the schematic also shows a `BC_State` related signal. Do not freely drive it as a normal GPIO.
- `GPIO12/GPIO13` are already connected to USB Type-C and are not recommended for use as general-purpose GPIO.
- `GPIO9` is the BOOT pin, and `EN/CHIP_PU` is the reset signal; they are not recommended as normal user inputs.
- The LCD and TF card share SPI clock and MOSI: `SCK=GPIO11`, `MOSI=GPIO21`; the TF card additionally uses `MISO=GPIO22` and `CS=GPIO15`.

## 7. References

| Type | Link |
|---|---|
| Chinese Wiki | https://docs.waveshare.net/ESP32-C6-Touch-LCD-1.28/ |
| English Documentation | https://docs.waveshare.com/ESP32-C6-Touch-LCD-1.28 |
| Schematic | `hardware/schematics/ESP32-C6-Touch-LCD-1.28.pdf` |
| Board Support Package (BSP) | `examples/ESP-IDF-v5.5.2/08_LVGL_Demo/components/esp32_c6_touch_lcd_1_28` |
