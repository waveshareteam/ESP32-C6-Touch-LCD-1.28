# Factory Firmware

[English](README.md) | [简体中文](README_ZH.md)

This directory stores the tested factory image for the ESP32-C6-Touch-LCD-1.28 development board.

| File | Purpose |
| --- | --- |
| [ESP32-C6-Touch-LCD-1.28_FactoryOnly_20260821.bin](ESP32-C6-Touch-LCD-1.28_FactoryOnly_20260821.bin) | Factory recovery firmware for the ESP32-C6-Touch-LCD-1.28 board, dated 20260821. Flash from `0x0`. |

Before flashing:

- Back up any device-specific data that must be preserved.
- Confirm the board model and hardware revision.
- Erase the target device before writing the merged image.
- Follow the flashing procedure in the product documentation.

SHA-256: `8FF1390B73E5483DB4EEFB1B0B7D6B09E73117F1570F75535657A29689F77FC6`

This file is a single merged BIN for factory recovery.
