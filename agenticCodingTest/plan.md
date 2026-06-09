# Implementation Plan

## 1. Toolchain Baseline - Done

Install or locate a working ESP-IDF toolchain and confirm `idf.py`, the ESP32 target tools, Python environment, and serial permissions are usable. Record the exact setup commands that work on this machine so later build and flash steps are repeatable.

Completed with ESP-IDF `v5.5.4` installed at `/home/mpetrick/.local/opt/esp-idf-v5.5.4`. Verified `idf.py --version`, `xtensa-esp32-elf-gcc`, Python dependencies, and `/dev/ttyACM0` access. Use:

```sh
source /home/mpetrick/.local/opt/esp-idf-v5.5.4/export.sh
```

## 2. Project Skeleton

Create the native ESP-IDF project structure with top-level CMake files, `main/`, `sdkconfig.defaults`, and a 16 MB-aware partition table. Keep the first skeleton minimal: it should compile for `esp32`, boot, and log a clear startup message before any display or game code is added.

## 3. Board Configuration

Define all board-specific constants in one place: display size, SPI pins, display control pins, backlight pin, button pins, serial baud rate, and flash assumptions. Start from the known TTGO/LILYGO T-Display pinout, then verify against hardware behavior during the first flash.

## 4. ST7789 Display Driver

Implement a small ESP-IDF SPI driver for the ST7789 display. The first version should support reset/init, orientation, backlight control, full-screen fill, drawing pixels, drawing rectangles, and pushing a rectangular pixel buffer.

## 5. Display Smoke Test Firmware

Build a smoke test that fills the screen with red, green, blue, black, and white, then draws a few rectangles and text-like debug patterns. Flash it to confirm the display pins, dimensions, color byte order, orientation, and backlight behavior.

## 6. Button Input Driver

Add GPIO input handling for the two onboard buttons with pull-up configuration, debounce, press/release events, and stable left/right naming. Log events over serial and display simple visual feedback so the physical button mapping can be verified.

## 7. Game Loop Foundation

Create a fixed-rate loop that separates input sampling, game update, and rendering. Use ESP-IDF timers or FreeRTOS timing primitives, keep frame timing visible in logs during development, and make the loop simple enough to reuse for multiple small games.

## 8. Drawing API

Wrap the display driver with a small drawing layer for common game operations: clear screen, fill rectangle, outline rectangle, draw bitmap region, and simple text/debug rendering. Keep this API independent from any single game so later games can share it.

## 9. First Demo Game

Implement a tiny playable demo using only the two buttons and the drawing API. The game should prove animation, input, screen clearing, collision or scoring logic, and stable frame timing without needing Wi-Fi or persistent storage.

## 10. Flash And Monitor Workflow

Add documented commands for build, flash, monitor, clean, and erase-flash. Prefer the stable `/dev/serial/by-id/...` port and make the commands copyable so future iterations can be flashed quickly.

## 11. Hardware Verification Pass

After the first firmware flashes, verify display orientation, button mapping, reset behavior, serial logs, and whether the board behaves like the documented T-Display pinout. Update `device.md`, `vision.md`, or board constants if the hardware differs.

## 12. Commit Milestones

Commit after each working stage: skeleton build, display smoke test, button input, game loop, drawing API, and demo game. Each commit should use a conventional commit message and include enough detail to explain what changed and what was verified.
