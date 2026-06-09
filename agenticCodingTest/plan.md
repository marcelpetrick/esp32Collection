# Implementation Plan

## 1. Toolchain Baseline - Done

Install or locate a working ESP-IDF toolchain and confirm `idf.py`, the ESP32 target tools, Python environment, and serial permissions are usable. Record the exact setup commands that work on this machine so later build and flash steps are repeatable.

Completed with ESP-IDF `v5.5.4` installed at `/home/mpetrick/.local/opt/esp-idf-v5.5.4`. Verified `idf.py --version`, `xtensa-esp32-elf-gcc`, Python dependencies, and `/dev/ttyACM0` access. Use:

```sh
source /home/mpetrick/.local/opt/esp-idf-v5.5.4/export.sh
```

## 2. Project Skeleton - Done

Create the native ESP-IDF project structure with top-level CMake files, `main/`, `sdkconfig.defaults`, and a 16 MB-aware partition table. Keep the first skeleton minimal: it should compile for `esp32`, boot, and log a clear startup message before any display or game code is added.

Completed with a native ESP-IDF project, 16 MB partition table, startup-only `app_main.c`, and local build ignore rules. Verified with `idf.py build`, flashed to the board, and captured serial logs showing `tdisplay_games`, ESP-IDF `v5.5.4`, 16 MB flash, two cores, revision `v1.1`, and heartbeat output.

## 3. Board Configuration - Done

Define all board-specific constants in one place: display size, SPI pins, display control pins, backlight pin, button pins, serial baud rate, and flash assumptions. Start from the known TTGO/LILYGO T-Display pinout, then verify against hardware behavior during the first flash.

Completed with a `board_config` module that centralizes display, SPI, backlight, button, battery ADC, flash, and serial constants for the TENSTAR/T-Display ESP32 profile. Verified with `idf.py build`, flashed to the device, and captured serial logs showing the expected ST7789 135x240 display profile, GPIO mapping, 16 MB flash assumption, and 115200 baud setting.

## 4. ST7789 Display Driver - Done

Implement a small ESP-IDF SPI driver for the ST7789 display. The first version should support reset/init, orientation, backlight control, full-screen fill, drawing pixels, drawing rectangles, and pushing a rectangular pixel buffer.

Completed with a native `st7789_display` module supporting panel reset/init, backlight on/off, full-screen fill, single pixels, filled rectangles, and rectangular RGB565 buffer pushes. Verified with `idf.py build`, flashed to the board, and captured serial logs showing successful ST7789 initialization, display clear, and heartbeat output. During bring-up the SPI clock was lowered from 40 MHz to 20 MHz because ESP-IDF rejects 40 MHz on this ESP32 GPIO-matrix route.

## 5. Display Smoke Test Firmware - Done

Build a smoke test that fills the screen with red, green, blue, black, and white, then draws a few rectangles and text-like debug patterns. Flash it to confirm the display pins, dimensions, color byte order, orientation, and backlight behavior.

Completed with a startup smoke test that cycles red, green, blue, white, and black fills, then draws a final diagnostic pattern with borders, grid lines, color swatches, diagonal pixels, an RGB565 checkerboard buffer push, and block-letter markers. Verified with `idf.py build`, flashed to the board, and captured serial logs for every smoke-test phase through final heartbeat output.

## 6. Button Input Driver - Done

Add GPIO input handling for the two onboard buttons with pull-up configuration, debounce, press/release events, and stable left/right naming. Log events over serial and display simple visual feedback so the physical button mapping can be verified.

Completed with a `button_input` module that configures left `GPIO0` and right `GPIO35`, applies 30 ms debounce, emits press/release events, and updates small on-screen button indicators. Verified with `idf.py build`, flashed to the board, and captured serial logs showing both GPIOs configured and the polling loop surviving multiple heartbeat intervals. Physical press mapping still needs a manual press test at the device.

## 7. Game Loop Foundation - Done

Create a fixed-rate loop that separates input sampling, game update, and rendering. Use ESP-IDF timers or FreeRTOS timing primitives, keep frame timing visible in logs during development, and make the loop simple enough to reuse for multiple small games.

Completed with a `game_loop` module that runs at a configured target FPS, samples button input, calls separate input/update/render callbacks, and logs frame-window timing stats. Verified with `idf.py build`, flashed to the board, and captured serial logs showing the fixed-rate loop running at 30 FPS across multiple stats windows.

## 8. Drawing API - Done

Wrap the display driver with a small drawing layer for common game operations: clear screen, fill rectangle, outline rectangle, draw bitmap region, and simple text/debug rendering. Keep this API independent from any single game so later games can share it.

Completed with a `graphics` module that wraps clear, filled rectangles, outlined rectangles, RGB565 bitmap regions, and scaled 5x7 text drawing. Integrated it into the render callback for on-screen button indicators. Verified with `idf.py build`, flashed to the board, and captured serial logs showing stable 30 FPS timing with the graphics-backed render path active.

## 9. First Demo Game - Done

Implement a tiny playable demo using only the two buttons and the drawing API. The game should prove animation, input, screen clearing, collision or scoring logic, and stable frame timing without needing Wi-Fi or persistent storage.

Completed with a two-button catch game. The left and right buttons move a paddle, a falling item advances every frame, collision increments score, and misses reset score. Rendering uses the drawing API with dirty rectangle updates and scaled text for the score. Verified with `idf.py build`, flashed to the board, and captured serial logs showing button press/release events and stable 30 FPS timing across multiple stats windows.

## 10. Flash And Monitor Workflow

Add documented commands for build, flash, monitor, clean, and erase-flash. Prefer the stable `/dev/serial/by-id/...` port and make the commands copyable so future iterations can be flashed quickly.

## 11. Hardware Verification Pass

After the first firmware flashes, verify display orientation, button mapping, reset behavior, serial logs, and whether the board behaves like the documented T-Display pinout. Update `device.md`, `vision.md`, or board constants if the hardware differs.

## 12. Commit Milestones

Commit after each working stage: skeleton build, display smoke test, button input, game loop, drawing API, and demo game. Each commit should use a conventional commit message and include enough detail to explain what changed and what was verified.
