# ESP32 T-Display Game Firmware Vision

## Goal

Build a native ESP-IDF firmware project for the identified LILYGO / TTGO T-Display ESP32 board so custom games and drawing experiments can be created, flashed, and tested reliably.

## Target Hardware

- Board: likely LILYGO / TTGO T-Display ESP32, CH9102, 16 MB flash variant
- MCU: ESP32-D0WDQ6, dual-core, 240 MHz capable
- Display: ST7789 TFT, 135 x 240, SPI
- Inputs: two onboard buttons
- Serial upload path: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00`
- Monitor baud: `115200`
- Flash: 16 MB, DIO, 80 MHz
- PSRAM: disabled unless later proven otherwise

## Direction

Use **ESP-IDF directly**, without Arduino.

Reasons:

- Official Espressif SDK and tooling.
- Clear control over FreeRTOS tasks, dual-core behavior, SPI, GPIO, timers, NVS, partitions, power modes, and flashing.
- Fewer wrapper layers than PlatformIO.
- Better fit for building a maintainable firmware base that can grow into games.

## First Milestone

Create a minimal ESP-IDF project that:

- Boots cleanly on the board.
- Initializes the ST7789 display.
- Fills the screen with test colors.
- Draws basic text, rectangles, and pixels.
- Reads both onboard buttons.
- Logs button events over serial.
- Uses a simple fixed-rate render loop suitable for games.
- Flashes with `idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00 flash monitor`.

## Project Shape

Expected initial structure:

- `CMakeLists.txt`
- `main/`
- `main/CMakeLists.txt`
- `main/app_main.c`
- `main/display_st7789.*`
- `main/input_buttons.*`
- `main/game_loop.*`
- `partitions.csv`
- `sdkconfig.defaults`

## Development Priorities

- Keep the first firmware small and verifiable.
- Prefer direct ESP-IDF APIs for SPI, GPIO, timers, logging, and tasks.
- Use a simple display abstraction that supports pixel, fill, rectangle, and text primitives before adding sprites.
- Keep game logic separate from board drivers.
- Use a partition table that acknowledges the 16 MB flash, rather than being limited by the existing 4 MB factory layout.
- Commit after each working milestone.

## Manual Input Needed Later

- Confirm whether the physical board markings say LILYGO, TTGO, or another clone name.
- Confirm whether the display lights and draws correctly after the first flash.
- Confirm button behavior if the left/right mapping needs to be swapped.
- Decide whether future games need Wi-Fi/Bluetooth, sound, battery display, persistent scores, or OTA updates.
