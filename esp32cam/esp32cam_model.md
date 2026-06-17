# ESP32-CAM Hardware Info

## Board

- **Module**: AI-Thinker ESP32-CAM
- **Programmer**: ESP32-CAM-MB (Micro Base board with CH340 USB-serial converter)
- **Camera**: OV2640 (connected via flat ribbon cable)

## Chip

| Property         | Value                          |
|------------------|-------------------------------|
| Chip             | ESP32-D0WDQ6 revision v1.0    |
| Cores            | Dual Core + LP Core           |
| Clock            | 240 MHz                       |
| Crystal          | 40 MHz                        |
| Wi-Fi            | Yes                           |
| Bluetooth        | Yes                           |
| MAC address      | 40:22:d8:03:5b:2c             |

## Flash

| Property         | Value         |
|------------------|---------------|
| Size             | 4 MB          |
| Voltage          | 3.3 V         |
| Manufacturer ID  | 0xD8 (Winbond)|
| Device ID        | 0x4016        |

## Connection

- **Port**: `/dev/ttyUSB0`
- **USB-serial**: QinHeng CH340 (1a86:7523)
- **Flash baud rate**: 115200

## Flashing

Boot mode requires manual trigger on the MB board:
1. Press and hold **BOOT** button
2. Press and release **RST** button
3. Release **BOOT**
4. Run esptool within ~1 second

```bash
esptool --port /dev/ttyUSB0 --baud 921600 write-flash 0x0 firmware.bin
```
