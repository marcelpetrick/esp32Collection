#!/usr/bin/env bash
# Build, flash, and monitor helper for the ESP32 project.
# Usage: ./build_flash.sh [build|flash|monitor|deploy|all]
#   build   – compile only (default)
#   flash   – flash the last build to the device
#   monitor – open serial monitor
#   deploy  – build then flash
#   all     – build, flash, then monitor (Ctrl-] to exit monitor)
set -euo pipefail

IDF_PATH="${IDF_PATH:-$HOME/.local/opt/esp-idf-v5.5.4}"
PORT="${PORT:-/dev/ttyACM0}"
FLASH_BAUD="${FLASH_BAUD:-460800}"
MONITOR_BAUD="${MONITOR_BAUD:-115200}"

# Confirm device exists before flashing
check_device() {
    if [[ ! -c "$PORT" ]]; then
        echo "ERROR: Serial device '$PORT' not found." >&2
        echo "       Connect the ESP32 or set PORT=/dev/ttyXXX" >&2
        exit 1
    fi
    if [[ ! -r "$PORT" || ! -w "$PORT" ]]; then
        echo "ERROR: No read/write access to '$PORT'." >&2
        echo "       Run: sudo setfacl -m u:$USER:rw $PORT" >&2
        exit 1
    fi
}

# Source IDF only once (idempotent-ish)
if ! command -v idf.py &>/dev/null; then
    # shellcheck source=/dev/null
    source "$IDF_PATH/export.sh"
fi

CMD="${1:-build}"

case "$CMD" in
    build)
        echo "==> Building..."
        idf.py build
        ;;
    flash)
        check_device
        echo "==> Flashing to $PORT at ${FLASH_BAUD} baud..."
        idf.py -p "$PORT" -b "$FLASH_BAUD" flash
        ;;
    monitor)
        check_device
        echo "==> Monitoring $PORT at ${MONITOR_BAUD} baud (Ctrl-] to exit)..."
        idf.py -p "$PORT" -b "$MONITOR_BAUD" monitor
        ;;
    deploy)
        echo "==> Building..."
        idf.py build
        check_device
        echo "==> Flashing to $PORT at ${FLASH_BAUD} baud..."
        idf.py -p "$PORT" -b "$FLASH_BAUD" flash
        echo "==> Done. Run './build_flash.sh monitor' to watch output."
        ;;
    all)
        echo "==> Building..."
        idf.py build
        check_device
        echo "==> Flashing to $PORT at ${FLASH_BAUD} baud..."
        idf.py -p "$PORT" -b "$FLASH_BAUD" flash
        echo "==> Monitoring $PORT at ${MONITOR_BAUD} baud (Ctrl-] to exit)..."
        idf.py -p "$PORT" -b "$MONITOR_BAUD" monitor
        ;;
    *)
        echo "Usage: $0 [build|flash|monitor|deploy|all]"
        exit 1
        ;;
esac
