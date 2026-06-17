#!/usr/bin/env bash
set -u
set -o pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IDF_PATH_ROOT="/home/mpetrick/.local/opt/esp-idf-v5.5.4"
DEVICE="/dev/ttyUSB0"
CACHE_FILE="${ROOT_DIR}/.pipeline-cache"
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

CACHE_SOURCE_HASH=""
CACHE_SOURCE_LINT_PASS=0
CACHE_DEPS_HASH=""
CACHE_DEPS_PASS=0

print_usage() {
    cat <<EOF
Usage: ./localPipeline.sh [--flash] [--monitor] [--help]

Local ESP-IDF pipeline for esp32cam (dillycam):
  1. Source IDF environment and verify idf.py is available
  2. clang-format check on main/*.c and main/*.h   (skipped if sources unchanged)
  3. cppcheck static analysis on main/              (skipped if sources unchanged)
  4. idf.py build — incremental via cmake/ninja
  5. Component dependency hash check               (skipped if deps unchanged)
  6. Firmware size report (DRAM/IRAM/flash)
  7. Flash to /dev/ttyUSB0 (only with --flash; hold BOOT+RST before running)
  8. Monitor serial output (only with --monitor)
  9. Print final summary

Incremental cache is stored in .pipeline-cache and skips lint/dep stages
when the relevant files have not changed since the last successful run.

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

compute_source_hash() {
    find "${ROOT_DIR}/main" -maxdepth 1 \( -name "*.c" -o -name "*.h" \) -print0 \
        | sort -z | xargs -0 md5sum 2>/dev/null \
        | md5sum | awk '{print $1}'
}

compute_deps_hash() {
    local files=()
    [[ -f "${ROOT_DIR}/main/idf_component.yml" ]] && files+=("${ROOT_DIR}/main/idf_component.yml")
    [[ -f "${ROOT_DIR}/dependencies.lock" ]]       && files+=("${ROOT_DIR}/dependencies.lock")
    if [[ "${#files[@]}" -eq 0 ]]; then
        printf 'no-deps-files\n'
        return
    fi
    md5sum "${files[@]}" 2>/dev/null | md5sum | awk '{print $1}'
}

load_cache() {
    if [[ -f "${CACHE_FILE}" ]]; then
        # shellcheck source=/dev/null
        source "${CACHE_FILE}" 2>/dev/null || true
    fi
}

save_cache() {
    local src_hash="$1"
    local src_pass="$2"
    local deps_hash="$3"
    local deps_pass="$4"
    cat > "${CACHE_FILE}" <<EOF
CACHE_SOURCE_HASH="${src_hash}"
CACHE_SOURCE_LINT_PASS=${src_pass}
CACHE_DEPS_HASH="${deps_hash}"
CACHE_DEPS_PASS=${deps_pass}
EOF
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
    log "Running idf.py build (incremental)."
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

    if [[ ! -f "${ROOT_DIR}/dependencies.lock" ]]; then
        DEPS_DETAILS="dependencies.lock missing; run: idf.py update-dependencies"
        return 1
    fi

    local lock_before lock_after
    lock_before="$(md5sum "${ROOT_DIR}/dependencies.lock" | awk '{print $1}')"

    run_with_log "${log_path}" idf.py -C "${ROOT_DIR}" update-dependencies

    lock_after="$(md5sum "${ROOT_DIR}/dependencies.lock" | awk '{print $1}')"

    if [[ "${lock_before}" == "${lock_after}" ]]; then
        DEPS_DETAILS="dependencies consistent with lock"
        return 0
    fi

    DEPS_DETAILS="lock was stale and has been updated; commit the new dependencies.lock"
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

run_clang_format_check_bg() {
    local out_file="$1"
    local ret_file="$2"
    run_clang_format_check
    local ret=$?
    printf '%s\n' "${CLANG_FORMAT_DETAILS}" > "${out_file}"
    printf '%d\n' "${ret}" > "${ret_file}"
}

run_cppcheck_bg() {
    local out_file="$1"
    local ret_file="$2"
    run_cppcheck
    local ret=$?
    printf '%s\n' "${CPPCHECK_DETAILS}" > "${out_file}"
    printf '%d\n' "${ret}" > "${ret_file}"
}

main() {
    local exit_code=1
    local clang_format_ret cppcheck_ret
    local current_source_hash current_deps_hash
    local lint_cached=false deps_cached=false

    parse_arguments "$@"
    load_cache

    if check_idf_environment; then
        IDF_ENV_OK=1
        mark_result "IDF Environment" "PASS" "idf.py available after sourcing export.sh"
    else
        mark_result "IDF Environment" "FAIL" "Could not source IDF or idf.py not found"
    fi

    if [[ "${IDF_ENV_OK}" -eq 1 ]]; then
        mkdir -p "${PIPELINE_LOG_DIR}"

        current_source_hash="$(compute_source_hash)"
        current_deps_hash="$(compute_deps_hash)"

        if [[ "${CACHE_SOURCE_LINT_PASS}" -eq 1 && "${current_source_hash}" == "${CACHE_SOURCE_HASH}" ]]; then
            lint_cached=true
            log "Lint cache hit — sources unchanged, skipping clang-format and cppcheck."
            CLANG_FORMAT_OK=1
            CPPCHECK_OK=1
            mark_result "clang-format" "PASS" "0 violations (cached — sources unchanged)"
            mark_result "cppcheck" "PASS" "0 findings (cached — sources unchanged)"
        else
            local cf_out="${PIPELINE_LOG_DIR}/cf_details" cf_ret="${PIPELINE_LOG_DIR}/cf_ret"
            local cpp_out="${PIPELINE_LOG_DIR}/cpp_details" cpp_ret="${PIPELINE_LOG_DIR}/cpp_ret"

            run_clang_format_check_bg "${cf_out}" "${cf_ret}" &
            local cf_pid=$!
            run_cppcheck_bg "${cpp_out}" "${cpp_ret}" &
            local cpp_pid=$!

            wait "${cf_pid}"
            clang_format_ret="$(cat "${cf_ret}" 2>/dev/null || printf '1')"
            CLANG_FORMAT_DETAILS="$(cat "${cf_out}" 2>/dev/null || printf 'no output')"

            wait "${cpp_pid}"
            cppcheck_ret="$(cat "${cpp_ret}" 2>/dev/null || printf '1')"
            CPPCHECK_DETAILS="$(cat "${cpp_out}" 2>/dev/null || printf 'no output')"

            if [[ "${clang_format_ret}" -eq 2 ]]; then
                mark_result "clang-format" "SKIP" "${CLANG_FORMAT_DETAILS}"
            elif [[ "${clang_format_ret}" -eq 0 ]]; then
                CLANG_FORMAT_OK=1
                mark_result "clang-format" "PASS" "${CLANG_FORMAT_DETAILS}"
            else
                mark_result "clang-format" "FAIL" "${CLANG_FORMAT_DETAILS}"
            fi

            if [[ "${cppcheck_ret}" -eq 2 ]]; then
                mark_result "cppcheck" "SKIP" "${CPPCHECK_DETAILS}"
            elif [[ "${cppcheck_ret}" -eq 0 ]]; then
                CPPCHECK_OK=1
                mark_result "cppcheck" "PASS" "${CPPCHECK_DETAILS}"
            else
                mark_result "cppcheck" "FAIL" "${CPPCHECK_DETAILS}"
            fi
        fi

        if run_build; then
            BUILD_OK=1
            mark_result "Build" "PASS" "${BUILD_DETAILS}"
        else
            mark_result "Build" "FAIL" "${BUILD_DETAILS}"
        fi

        if [[ "${BUILD_OK}" -eq 1 ]]; then
            if [[ "${CACHE_DEPS_PASS}" -eq 1 && "${current_deps_hash}" == "${CACHE_DEPS_HASH}" ]]; then
                deps_cached=true
                log "Dep cache hit — idf_component.yml and dependencies.lock unchanged."
                DEPS_OK=1
                mark_result "Dep Hash Check" "PASS" "dependencies consistent with lock (cached)"
            elif run_dependency_check; then
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

        local new_lint_pass=0
        [[ "${CLANG_FORMAT_OK}" -eq 1 && "${CPPCHECK_OK}" -eq 1 ]] && new_lint_pass=1

        local new_deps_pass=0
        [[ "${DEPS_OK}" -eq 1 ]] && new_deps_pass=1

        local saved_source_hash="${current_source_hash}"
        local saved_deps_hash="${current_deps_hash}"
        [[ "${lint_cached}" == true ]] && saved_source_hash="${CACHE_SOURCE_HASH}"
        [[ "${deps_cached}" == true ]] && saved_deps_hash="${CACHE_DEPS_HASH}"

        save_cache "${saved_source_hash}" "${new_lint_pass}" "${saved_deps_hash}" "${new_deps_pass}"
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
