# Device Investigation

## Summary

The connected USB device is a WCH/QinHeng CH9102-class USB-to-UART serial converter, not enough by itself to identify the target microcontroller or board behind the serial adapter.

The adapter is exposed on Linux as `/dev/ttyACM0` via the `cdc_acm` driver.

Serial probing of the running firmware was not possible in this session because `/dev/ttyACM0` is owned by `root:uucp` with mode `660`, and the current user is not a member of `uucp`.

## Observed USB Device

- USB vendor/product: `1a86:55d4`
- Vendor: QinHeng Electronics / WCH
- Product string: `USB Single Serial`
- USB serial number: `5B0A006803`
- Kernel driver: `cdc_acm`
- TTY device: `/dev/ttyACM0`
- Stable by-id path: `/dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00`
- USB class: CDC ACM serial
- Negotiated USB speed: Full Speed, 12 Mbps
- Reported USB device revision: `4.44`

## Device Identity

The USB ID `1a86:55d4` is listed as `CH9102 serial converter` in the USB ID database. WCH's Linux driver documentation also describes the CH9102 family as USB-to-UART chips that can work with the standard Linux `cdc_acm` driver.

This means the directly visible USB device is the serial bridge. The actual attached device could be an ESP32, another microcontroller, or another UART target, but it cannot be confirmed from the USB descriptor alone.

## Firmware Capabilities

Current firmware capabilities could not be confirmed.

What was checked:

- The serial adapter is present and enumerates correctly.
- The repository currently contains no firmware source, build config, binaries, or project metadata that would identify the current firmware.
- No serial console output or command response could be read because opening `/dev/ttyACM0` failed with `Permission denied`.
- No evidence was found for application capabilities, command protocol, analog I/O support, wireless features, sensor support, or board type.

## Access Blocker

Current device permissions:

```text
crw-rw---- 1 root uucp ... /dev/ttyACM0
```

Current user groups do not include `uucp`.

To test the firmware over serial, add the user to `uucp` and start a new login session, or temporarily change the device permissions:

```sh
sudo usermod -aG uucp "$USER"
```

After reconnecting or starting a new session, a basic probe can be retried with:

```sh
picocom -b 115200 /dev/serial/by-id/usb-1a86_USB_Single_Serial_5B0A006803-if00
```

Other common baud rates to try if there is no output: `9600`, `74880`, and `921600`.

## Sources

- Local `lsusb -v` and `udevadm info` output from the connected device.
- Local `/dev/serial/by-id` symlink inspection.
- USB ID database entry for `1a86:55d4`: `https://usb-ids.gowdy.us/read/UD/1a86/55d4`
- WCH Linux driver documentation: `https://github.com/WCHSoftGroup/ch343ser_linux`
