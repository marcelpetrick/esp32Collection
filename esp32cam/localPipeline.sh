#!/usr/bin/env bash
set -u
set -o pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IDF_PATH_ROOT="/home/mpetrick/.local/opt/esp-idf-v5.5.4"
DEVICE="/dev/ttyUSB0"
PIPELINE_LOG_DIR="${TMPDIR:-/tmp}/esp32cam-pipeline-$$"
trap 'rm -rf "${PIPELINE_LOG_DIR}"' EXIT

declare -a SUMMARY_LINES=()

DO_FLASH=false
DO_MONITOR=false

IDF_ENV_OK=0
CLANG_FORMAT_OK=0
CPPCHECK_OK=0
BUILD_OK=0
DEPS_OK=0
SIZE_OK=0
FLASH_OK=0
MONITOR_OK=0

CLANG_FORMAT_DETAILS=""
CPPCHECK_DETAILS=""
BUILD_DETAILS=""
DEPS_DETAILS=""
SIZE_DETAILS=""

print_usage() {
    cat <<EOF
Usage: ./localPipeline.sh [--flash] [--monitor] [--help]

Local ESP-IDF pipeline for esp32cam (dillycam):
  1. Source IDF environment and verify idf.py is available
  2. clang-format check on main/*.c and main/*.h
  3. cppcheck static analysis on main/
  4. idf.py build — full firmware build
  5. Component dependency hash check (idf.py update-dependencies --check)
  6. Firmware size report (DRAM/IRAM/flash)
  7. Flash to /dev/ttyUSB0 (only with --flash; hold BOOT+RST before running)
  8. Monitor serial output (only with --monitor)
  9. Print final summary

Options:
  --flash    Flash firmware to ${DEVICE} after a successful build
  --monitor  Open serial monitor on ${DEVICE}
  --help     Show this help and exit
EOF
}

log() {
    printf '[INFO] %s\n' "$*"
}

warn() {
    printf '[WARN] %s\n' "$*" >&2
}

error() {
    printf '[ERROR] %s\n' "$*" >&2
}

mark_result() {
    local label="$1"
    local status="$2"
    local details="$3"
    SUMMARY_LINES+=("$(printf '%-20s : %-4s %s' "${label}" "${status}" "${details}")")
}

run_with_log() {
    local log_path="$1"
    shift

    mkdir -p "${PIPELINE_LOG_DIR}"
    "$@" 2>&1 | tee "${log_path}"
    return "${PIPESTATUS[0]}"
}

print_summary() {
    printf '\n========== Local Pipeline Summary ==========\n'
    local line
    for line in "${SUMMARY_LINES[@]}"; do
        printf '%s\n' "${line}"
    done
    printf '=============================================\n'
}

parse_arguments() {
    for argument in "$@"; do
        case "${argument}" in
            --flash)
                DO_FLASH=true
                ;;
            --monitor)
                DO_MONITOR=true
                ;;
            --help|-h)
                print_usage
                exit 0
                ;;
            *)
                error "Unknown argument: ${argument}"
                print_usage
                exit 2
                ;;
        esac
    done
}

source_idf_environment() {
    log "Sourcing IDF environment from ${IDF_PATH_ROOT}/export.sh"
    # shellcheck source=/dev/null
    set +u
    source "${IDF_PATH_ROOT}/export.sh"
    set -u
}

check_idf_environment() {
    log "Checking IDF environment."

    if [[ ! -f "${IDF_PATH_ROOT}/export.sh" ]]; then
        error "IDF export.sh not found at ${IDF_PATH_ROOT}/export.sh"
        return 1
    fi

    if ! source_idf_environment; then
        error "Sourcing IDF export.sh failed."
        return 1
    fi

    if ! command -v idf.py >/dev/null 2>&1; then
        error "idf.py not found on PATH after sourcing export.sh."
        return 1
    fi

    local idf_version
    idf_version="$(idf.py --version 2>&1 | head -n 1 || true)"
    log "idf.py found: ${idf_version}"
    return 0
}

extract_clang_format_details() {
    local log_path="$1"
    local violation_count
    violation_count="$(grep -c "error:" "${log_path}" 2>/dev/null || true)"
    if [[ "${violation_count}" -eq 0 ]]; then
        printf '%s\n' "0 violations"
    else
        printf '%s violation(s)\n' "${violation_count}"
    fi
}

run_clang_format_check() {
    log "Running clang-format check on main/*.c and main/*.h"
    local log_path="${PIPELINE_LOG_DIR}/clang-format.log"

    if ! command -v clang-format >/dev/null 2>&1; then
        CLANG_FORMAT_DETAILS="clang-format not found; install with: sudo pacman -S clang"
        return 2
    fi

    local files=()
    while IFS= read -r -d '' f; do
        files+=("${f}")
    done < <(find "${ROOT_DIR}/main" -maxdepth 1 \( -name "*.c" -o -name "*.h" \) -print0)

    if [[ "${#files[@]}" -eq 0 ]]; then
        CLANG_FORMAT_DETAILS="no .c or .h files found in main/"
        return 0
    fi

    if run_with_log "${log_path}" clang-format --dry-run --Werror "${files[@]}"; then
        CLANG_FORMAT_DETAILS="$(extract_clang_format_details "${log_path}")"
        return 0
    fi

    CLANG_FORMAT_DETAILS="$(extract_clang_format_details "${log_path}")"
    return 1
}

extract_cppcheck_details() {
    local log_path="$1"
    local finding_count
    finding_count="$(grep -cE ": (error|warning|style|performance):" "${log_path}" 2>/dev/null || true)"
    if [[ "${finding_count}" -eq 0 ]]; then
        printf '%s\n' "0 findings"
    else
        printf '%s finding(s)\n' "${finding_count}"
    fi
}

run_cppcheck() {
    log "Running cppcheck static analysis on main/"
    local log_path="${PIPELINE_LOG_DIR}/cppcheck.log"

    if ! command -v cppcheck >/dev/null 2>&1; then
        CPPCHECK_DETAILS="cppcheck not found; install with: sudo pacman -S cppcheck"
        return 2
    fi

    local suppress_args=()
    if [[ -f "${ROOT_DIR}/.cppcheck-suppress" ]]; then
        suppress_args=(--suppressions-list="${ROOT_DIR}/.cppcheck-suppress")
    fi

    if run_with_log "${log_path}" cppcheck \
        --enable=warning,style,performance \
        --error-exitcode=1 \
        "${suppress_args[@]}" \
        "${ROOT_DIR}/main/"; then
        CPPCHECK_DETAILS="$(extract_cppcheck_details "${log_path}")"
        return 0
    fi

    CPPCHECK_DETAILS="$(extract_cppcheck_details "${log_path}")"
    return 1
}

extract_build_size() {
    local bin_path="${ROOT_DIR}/build/dillycam.bin"
    if [[ -f "${bin_path}" ]]; then
        local size_bytes
        size_bytes="$(wc -c < "${bin_path}")"
        printf '%s bytes (%.1f KB)\n' "${size_bytes}" "$(awk "BEGIN { printf \"%.1f\", ${size_bytes}/1024 }")"
    else
        printf '%s\n' "binary not found"
    fi
}

run_build() {
    log "Running idf.py build."
    local log_path="${PIPELINE_LOG_DIR}/build.log"

    if run_with_log "${log_path}" idf.py -C "${ROOT_DIR}" build; then
        BUILD_DETAILS="$(extract_build_size)"
        return 0
    fi

    BUILD_DETAILS="build failed; see output above"
    return 1
}

run_dependency_check() {
    log "Checking managed component dependencies."
    local log_path="${PIPELINE_LOG_DIR}/deps.log"

    if [[ ! -f "${ROOT_DIR}/main/idf_component.yml" ]]; then
        DEPS_DETAILS="no idf_component.yml found in main/"
        return 1
    fi

    if run_with_log "${log_path}" idf.py -C "${ROOT_DIR}" update-dependencies --check; then
        DEPS_DETAILS="dependencies consistent with lock"
        return 0
    fi

    DEPS_DETAILS="dependency lock mismatch; run: idf.py update-dependencies"
    return 1
}

extract_size_details() {
    local log_path="$1"
    local dram iram flash_total

    dram="$(grep -oE "DRAM .Used[^,]*" "${log_path}" | head -n 1 || true)"
    iram="$(grep -oE "IRAM .Used[^,]*" "${log_path}" | head -n 1 || true)"
    flash_total="$(grep -oE "Flash used.*$" "${log_path}" | head -n 1 || true)"

    if [[ -n "${dram}" || -n "${iram}" || -n "${flash_total}" ]]; then
        printf '%s | %s | %s\n' "${dram}" "${iram}" "${flash_total}"
    else
        local bin_size
        bin_size="$(extract_build_size)"
        printf 'binary: %s\n' "${bin_size}"
    fi
}

run_size_report() {
    log "Generating firmware size report."
    local log_path="${PIPELINE_LOG_DIR}/size.log"

    if run_with_log "${log_path}" idf.py -C "${ROOT_DIR}" size; then
        SIZE_DETAILS="$(extract_size_details "${log_path}")"
        return 0
    fi

    SIZE_DETAILS="idf.py size failed"
    return 1
}

run_flash() {
    log "Flashing firmware to ${DEVICE}."
    warn "Make sure to hold BOOT then press RST on the ESP32-CAM before the flash starts."
    local log_path="${PIPELINE_LOG_DIR}/flash.log"

    if [[ ! -c "${DEVICE}" ]]; then
        error "Device ${DEVICE} not found. Connect the board and try again."
        return 1
    fi

    if run_with_log "${log_path}" idf.py -C "${ROOT_DIR}" -p "${DEVICE}" flash; then
        return 0
    fi

    return 1
}

run_monitor() {
    log "Opening serial monitor on ${DEVICE}. Press Ctrl+] to exit."

    if [[ ! -c "${DEVICE}" ]]; then
        error "Device ${DEVICE} not found. Connect the board and try again."
        return 1
    fi

    idf.py -C "${ROOT_DIR}" -p "${DEVICE}" monitor
}

main() {
    local exit_code=1
    local clang_format_ret cppcheck_ret

    parse_arguments "$@"

    if check_idf_environment; then
        IDF_ENV_OK=1
        mark_result "IDF Environment" "PASS" "idf.py available after sourcing export.sh"
    else
        mark_result "IDF Environment" "FAIL" "Could not source IDF or idf.py not found"
    fi

    if [[ "${IDF_ENV_OK}" -eq 1 ]]; then
        clang_format_ret=0
        run_clang_format_check || clang_format_ret=$?
        if [[ "${clang_format_ret}" -eq 2 ]]; then
            mark_result "clang-format" "SKIP" "${CLANG_FORMAT_DETAILS}"
        elif [[ "${clang_format_ret}" -eq 0 ]]; then
            CLANG_FORMAT_OK=1
            mark_result "clang-format" "PASS" "${CLANG_FORMAT_DETAILS}"
        else
            mark_result "clang-format" "FAIL" "${CLANG_FORMAT_DETAILS}"
        fi

        cppcheck_ret=0
        run_cppcheck || cppcheck_ret=$?
        if [[ "${cppcheck_ret}" -eq 2 ]]; then
            mark_result "cppcheck" "SKIP" "${CPPCHECK_DETAILS}"
        elif [[ "${cppcheck_ret}" -eq 0 ]]; then
            CPPCHECK_OK=1
            mark_result "cppcheck" "PASS" "${CPPCHECK_DETAILS}"
        else
            mark_result "cppcheck" "FAIL" "${CPPCHECK_DETAILS}"
        fi

        if run_build; then
            BUILD_OK=1
            mark_result "Build" "PASS" "${BUILD_DETAILS}"
        else
            mark_result "Build" "FAIL" "${BUILD_DETAILS}"
        fi

        if [[ "${BUILD_OK}" -eq 1 ]]; then
            if run_dependency_check; then
                DEPS_OK=1
                mark_result "Dep Hash Check" "PASS" "${DEPS_DETAILS}"
            else
                mark_result "Dep Hash Check" "FAIL" "${DEPS_DETAILS}"
            fi

            if run_size_report; then
                SIZE_OK=1
                mark_result "Size Report" "PASS" "${SIZE_DETAILS}"
            else
                mark_result "Size Report" "WARN" "${SIZE_DETAILS}"
            fi
        else
            mark_result "Dep Hash Check" "SKIP" "Skipped because build failed"
            mark_result "Size Report" "SKIP" "Skipped because build failed"
        fi
    else
        mark_result "clang-format" "SKIP" "Skipped because IDF environment is unavailable"
        mark_result "cppcheck" "SKIP" "Skipped because IDF environment is unavailable"
        mark_result "Build" "SKIP" "Skipped because IDF environment is unavailable"
        mark_result "Dep Hash Check" "SKIP" "Skipped because IDF environment is unavailable"
        mark_result "Size Report" "SKIP" "Skipped because IDF environment is unavailable"
    fi

    if [[ "${DO_FLASH}" == true ]]; then
        if [[ "${BUILD_OK}" -eq 1 ]]; then
            if run_flash; then
                FLASH_OK=1
                mark_result "Flash" "PASS" "Firmware flashed to ${DEVICE}"
            else
                mark_result "Flash" "FAIL" "Flash failed; check connection and BOOT+RST timing"
            fi
        else
            mark_result "Flash" "SKIP" "Skipped because build failed"
        fi
    else
        mark_result "Flash" "SKIP" "Pass --flash to enable"
    fi

    if [[ "${DO_MONITOR}" == true ]]; then
        if [[ "${BUILD_OK}" -eq 1 ]]; then
            mark_result "Monitor" "PASS" "Monitor started; exited normally"
            run_monitor || mark_result "Monitor" "WARN" "Monitor exited with non-zero status"
            MONITOR_OK=1
        else
            mark_result "Monitor" "SKIP" "Skipped because build failed"
        fi
    else
        mark_result "Monitor" "SKIP" "Pass --monitor to enable"
    fi

    if [[ "${IDF_ENV_OK}" -eq 1 && "${BUILD_OK}" -eq 1 && "${DEPS_OK}" -eq 1 ]]; then
        exit_code=0
    fi

    if [[ "${exit_code}" -eq 0 ]]; then
        log "localPipeline.sh completed successfully"
    else
        error "localPipeline.sh completed with failing mandatory stage(s)"
    fi

    print_summary
    exit "${exit_code}"
}

main "$@"
