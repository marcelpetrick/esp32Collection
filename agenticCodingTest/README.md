# TENSTAR T-Display ESP32 Games

Native ESP-IDF firmware for the TENSTAR T-Display ESP32 clone documented in `device.md`.

## Environment

```sh
source /home/mpetrick/.local/opt/esp-idf-v5.5.4/export.sh
```

## Build

```sh
idf.py set-target esp32
idf.py build
```

## Flash And Monitor

```sh
idf.py -p /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00 flash monitor
```

## Factory Backup

A full 16 MB factory flash backup was written outside the repository:

```text
/tmp/tenstar-t-display-factory-16mb.bin
sha256: 09384a159808cd02e1666be189d9d377f1fcf7fe88ac8a9b0a82d91caf92cb28
```
