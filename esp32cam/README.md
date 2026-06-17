# dillyCam — ESP32-CAM HTTP Stream

Live MJPEG camera stream hosted directly from an ESP32-CAM over its own Wi-Fi access point. Open the stream URL on any device on the same network — no router needed.

## Hardware

| Component        | Details                                      |
|------------------|----------------------------------------------|
| Module           | AI-Thinker ESP32-CAM                         |
| Programmer       | ESP32-CAM-MB (Micro Base, CH340 USB-serial)  |
| Camera           | OV2640 (ribbon cable)                        |
| Chip             | ESP32-D0WDQ6 rev 1.0, dual core, 240 MHz     |
| Flash            | 4 MB Winbond, 3.3 V                          |
| MAC              | 40:22:d8:03:5b:2c                            |
| Serial port      | `/dev/ttyUSB0`                               |

## Wi-Fi Access Point

| Setting  | Value      |
|----------|------------|
| SSID     | dillyCam   |
| Password | foobar     |
| IP       | 192.168.4.1|

Connect your smartphone to the `dillyCam` network, then open:

```
http://192.168.4.1/stream
```

## Toolchain

Built with **ESP-IDF v5.5.4** in C. No Arduino, no MicroPython.

- Components: `espressif/esp32-camera` (managed, pinned in `dependencies.lock`), `esp_http_server`, `esp_wifi`

## Local Pipeline

```bash
./localPipeline.sh            # lint + build + dep check
./localPipeline.sh --flash    # also flash to /dev/ttyUSB0
./localPipeline.sh --monitor  # also open serial monitor
```

Stages and expected output on a clean tree:

```
========== Local Pipeline Summary ==========
IDF Environment      : PASS idf.py available after sourcing export.sh
clang-format         : PASS 0 violations
cppcheck             : PASS 0 findings
Build                : PASS 960832 bytes (938.3 KB)
Dep Hash Check       : PASS dependencies consistent with lock
Size Report          : PASS binary: 960832 bytes (938.3 KB)
Flash                : SKIP Pass --flash to enable
Monitor              : SKIP Pass --monitor to enable
=============================================
```

## Build & Flash

```bash
# Set up ESP-IDF environment (once per shell)
source ~/.local/opt/esp-idf-v5.5.4/export.sh

# Build
idf.py build

# Flash (put board in boot mode first: hold BOOT, tap RST, release BOOT)
idf.py -p /dev/ttyUSB0 flash

# Monitor serial output
idf.py -p /dev/ttyUSB0 monitor
```

### Boot mode (required for flashing)

1. Press and hold the **BOOT** button on the MB board
2. Press and release **RST**
3. Release **BOOT**
4. Run `idf.py flash` within ~1 second

## Project Structure

```
esp32cam/
├── main/
│   ├── main.c            # App entry — Wi-Fi AP + HTTP server
│   ├── camera.c/h        # OV2640 init (AI-Thinker pin mapping)
│   ├── stream.c/h        # MJPEG stream handler
│   └── CMakeLists.txt
├── CMakeLists.txt
├── sdkconfig.defaults    # PSRAM, partition table, HTTP server tuning
├── dependencies.lock     # Pinned component versions
├── .cppcheck-suppress    # cppcheck suppressions for ESP-IDF macros
├── .clang-format         # Google style, indent 4, col 100
├── localPipeline.sh      # Local CI pipeline
├── esp32cam_model.md     # Hardware probe results
└── README.md
```
