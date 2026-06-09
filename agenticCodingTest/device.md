# Device Investigation

## Identification

The connected board is a **TENSTAR T-Display ESP32 clone of the LILYGO / TTGO T-Display ESP32, CH9102, 16 MB flash variant**.

Evidence:

- USB serial bridge is WCH/QinHeng `CH9102` class: USB ID `1a86:55d4`, product `USB Single Serial`.
- Target MCU detected through the bridge is `ESP32-D0WDQ6`, revision `v1.1`.
- Flash detected by `esptool`: `16MB`.
- Current firmware strings and boot behavior match the T-Display factory test: Wi-Fi scan, voltage monitor, left/right buttons, display output, and deep sleep.
- LILYGO documents the T-Display CH9102 variant as ESP32 with Wi-Fi/Bluetooth, CH9102 serial, optional 4 MB/16 MB flash, buttons, battery voltage detection, and 1.14 inch ST7789 display.
- Purchase listing provided by the owner names it as `TENSTAR T-Display ESP32-D0WD CH9102 Chip 16 MB WiFi ... 1.14 inch LCD` with selected option `CH9102F 16MB`.
- A buyer review on that listing reports `ESP32-DOWDQ5`, `Winbond 25Q128JVSO (128 Mbit)`, `ST7789`, and `CH9102F`; this matches the local probe except the local chip probe identified `ESP32-D0WDQ6`.

## Access

- Serial device: `/dev/ttyACM0`
- Stable path: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00`
- USB serial number: `5B0A006803`
- Boot log baud rate: `115200`
- `esptool` probing worked at `115200`; flash reads worked at `460800`.

## ESP32 Details

- Chip: `ESP32-D0WDQ6`
- Revision: `v1.1`
- CPU: dual-core Xtensa LX6, rated for 240 MHz
- Crystal: `40MHz`
- MAC: `88:13:bf:fc:c9:74`
- Wireless hardware capability: Wi-Fi and Bluetooth/BLE
- ADC calibration: eFuse Vref present, `1114 mV`
- Flash: `16MB`, manufacturer `0x85`, device `0x2018`
- Flash voltage: `3.3V`
- Bootloader/app image flash mode: `DIO`, `80m`
- Secure boot: off
- Flash encryption: off
- UART download mode: enabled
- JTAG disabled: no
- PSRAM: not detected during probing; treat as disabled unless proven otherwise

## Factory Flash Layout Before Replacement

Partition table:

| Name | Type | Subtype | Offset | Size |
| --- | --- | --- | --- | --- |
| `nvs` | data | nvs | `0x9000` | `0x5000` |
| `otadata` | data | ota | `0xe000` | `0x2000` |
| `app0` | app | ota_0 | `0x10000` | `0x140000` |
| `app1` | app | ota_1 | `0x150000` | `0x140000` |
| `spiffs` | data | spiffs | `0x290000` | `0x170000` |

Notes:

- Physical flash is 16 MB, but the current partition layout only uses the first 4 MB.
- OTA data indicates `app0` is active (`seq=1`); `app1` is erased.
- `app0` SHA-256 dump hash: `dbb326c6078012e1bf9bd3498e8be130ee1b0e61bf6e7b66640c3d0a25b82b81`
- `app1` SHA-256 dump hash: `35805909a516a528e396b6ea22dab437b6f1c703afe92e92dddfa0243bfae738` (erased slot)

## Factory Firmware Before Replacement

The original installed application appeared to be a T-Display factory/demo firmware built with the Arduino ESP32 core over ESP-IDF. It has since been replaced by the native ESP-IDF development firmware in this repository.

Observed boot output at `115200`:

```text
rst:0x1 (POWERON_RESET),boot:0x13 (SPI_FAST_FLASH_BOOT)
mode:DIO, clock div:1
entry 0x4008069c
Start
```

Notable strings in the active app:

- `Start`
- `Scan Network`
- `btn press wifi scan`
- `[WiFi Scan]`
- `Detect Voltage..`
- `Voltage :`
- `[Voltage Monitor]`
- `Press again to wake up`
- `RightButtonLongPress:`
- `[Deep Sleep]`
- `LeftButton:`
- `RightButton:`
- `v3.2.3`
- `master`

Inferred firmware capabilities:

- Drives the onboard TFT display.
- Uses two onboard buttons.
- Scans Wi-Fi networks.
- Reads and displays battery/input voltage through ADC calibration.
- Enters deep sleep and wakes from a button.
- Includes SPIFFS support in the partition table.
- Includes OTA-capable partitioning, although only `app0` is currently populated.

No command-line serial console was found. Passive reads and simple `help`, `?`, `version`, `ver`, and `info` probes produced no application response.

## Current Development Firmware

The board is currently running the native ESP-IDF project in this repository:

- Project name: `tdisplay_games`
- ESP-IDF: `v5.5.4`
- Partition table: custom 16 MB layout from `partitions.csv`
- Display driver: native ST7789 SPI driver
- Display size in firmware: `135 x 240`
- ST7789 RAM offset in firmware: `x=52`, `y=40`
- Display SPI pins: MOSI `GPIO19`, SCLK `GPIO18`, CS `GPIO5`, DC `GPIO16`, RST `GPIO23`, BL `GPIO4`
- Display SPI clock: `20 MHz`
- Current verified behavior: boots, initializes ST7789, runs red/green/blue/white/black fill phases, leaves a final diagnostic pattern on screen, and emits heartbeat logs.

## NVS

NVS was dumped from `0x9000` length `0x5000` to `/tmp/esp32-nvs.bin`.

Visible key names include standard ESP32 Wi-Fi storage entries:

- `nvs.net80211`
- `sta.ssid`
- `sta.pswd`
- `sta.pmk`
- `sta.chan`
- `sta.bssid`
- `ap.ssid`
- `ap.passwd`
- `ap.pmk`
- `ap.chan`
- `cal_data`

The available local NVS tool can generate/encrypt/decrypt partitions but did not provide a plain decoder here, so only key names were extracted.

## Programming Notes

For future firmware work, target this as a **classic ESP32 T-Display clone**, not ESP32-S3/C3.

Recommended starting configuration:

- Board family: ESP32
- Board profile: `ESP32 Dev Module` or a `LILYGO TTGO T-Display` profile
- Flash size: `16MB`
- PSRAM: disabled
- Flash mode: `DIO`
- Flash frequency: `80MHz`
- Upload port: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00`
- Serial monitor: `115200`
- Display driver expectation: ST7789, 135 x 240, SPI
- Display SPI clock: start at `20 MHz`; ESP-IDF rejected `40 MHz` on the current GPIO-matrix route with a limit below `26.666 MHz`.

If reusing the current partition style, preserve the offsets above. If the project needs the full flash, define a new 16 MB partition table rather than assuming the current 4 MB layout uses all available flash.

## Sources

- Local `lsusb -v`, `udevadm`, `esptool`, `espefuse`, serial boot-log, and flash metadata output.
- USB ID database entry for `1a86:55d4`: `https://usb-ids.gowdy.us/read/UD/1a86/55d4`
- WCH Linux driver documentation: `https://github.com/WCHSoftGroup/ch343ser_linux`
- LILYGO T-Display product page: `https://lilygo.cc/products/t-display`
- T-Display factory-test behavior reference: `https://protosupplies.com/product/lilygo-esp32-t-display-development-board/`
- Owner-provided AliExpress listing details for TENSTAR T-Display ESP32-D0WD CH9102 16 MB variant.
