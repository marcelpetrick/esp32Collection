#include "game_loop.h"

#include <stdint.h>

#include "button_input.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define DEFAULT_TARGET_FPS 30
#define DEFAULT_STATS_INTERVAL_MS 5000

static const char *TAG = "game_loop";

static uint32_t elapsed_ms(uint64_t start_us, uint64_t end_us)
{
    return (uint32_t)((end_us - start_us) / 1000);
}

static void wait_until_us(uint64_t deadline_us)
{
    while (true) {
        const uint64_t now_us = esp_timer_get_time();
        if (now_us >= deadline_us) {
            return;
        }

        const uint64_t remaining_us = deadline_us - now_us;
        if (remaining_us > 2000) {
            vTaskDelay(pdMS_TO_TICKS((uint32_t)((remaining_us - 1000) / 1000)));
        } else {
            esp_rom_delay_us((uint32_t)remaining_us);
        }
    }
}

esp_err_t game_loop_run(button_input_t *buttons, const game_loop_config_t *config)
{
    ESP_RETURN_ON_FALSE(buttons != NULL, ESP_ERR_INVALID_ARG, TAG, "buttons are required");
    ESP_RETURN_ON_FALSE(config != NULL, ESP_ERR_INVALID_ARG, TAG, "config is required");

    const uint32_t target_fps = config->target_fps == 0 ? DEFAULT_TARGET_FPS : config->target_fps;
    const uint32_t stats_interval_ms = config->stats_interval_ms == 0 ? DEFAULT_STATS_INTERVAL_MS : config->stats_interval_ms;
    const uint32_t frame_period_us = 1000000 / target_fps;
    ESP_RETURN_ON_FALSE(frame_period_us > 0, ESP_ERR_INVALID_ARG, TAG, "target fps is too high");

    ESP_LOGI(TAG, "Starting fixed-rate loop at %lu FPS", (unsigned long)target_fps);

    uint64_t previous_frame_us = esp_timer_get_time();
    uint64_t stats_start_us = previous_frame_us;
    uint64_t next_frame_us = previous_frame_us;
    uint32_t frame = 0;
    uint32_t frames_in_window = 0;
    uint32_t max_input_ms = 0;
    uint32_t max_update_ms = 0;
    uint32_t max_render_ms = 0;

    while (true) {
        const uint64_t frame_start_us = esp_timer_get_time();
        const uint32_t dt_ms = elapsed_ms(previous_frame_us, frame_start_us);
        previous_frame_us = frame_start_us;

        button_event_t events[BUTTON_ID_COUNT];
        size_t event_count = 0;

        ESP_RETURN_ON_ERROR(button_input_poll(buttons, events, BUTTON_ID_COUNT, &event_count), TAG, "input poll failed");

        if (config->on_input != NULL && event_count > 0) {
            ESP_RETURN_ON_ERROR(config->on_input(events, event_count, config->ctx), TAG, "input callback failed");
        }
        const uint64_t input_callback_done_us = esp_timer_get_time();

        if (config->on_update != NULL) {
            ESP_RETURN_ON_ERROR(config->on_update(frame, dt_ms, config->ctx), TAG, "update callback failed");
        }
        const uint64_t update_done_us = esp_timer_get_time();

        if (config->on_render != NULL) {
            ESP_RETURN_ON_ERROR(config->on_render(frame, dt_ms, config->ctx), TAG, "render callback failed");
        }
        const uint64_t render_done_us = esp_timer_get_time();

        const uint32_t input_ms = elapsed_ms(frame_start_us, input_callback_done_us);
        const uint32_t update_ms = elapsed_ms(input_callback_done_us, update_done_us);
        const uint32_t render_ms = elapsed_ms(update_done_us, render_done_us);
        if (input_ms > max_input_ms) {
            max_input_ms = input_ms;
        }
        if (update_ms > max_update_ms) {
            max_update_ms = update_ms;
        }
        if (render_ms > max_render_ms) {
            max_render_ms = render_ms;
        }

        ++frame;
        ++frames_in_window;

        const uint64_t now_us = esp_timer_get_time();
        if (elapsed_ms(stats_start_us, now_us) >= stats_interval_ms) {
            const uint32_t window_ms = elapsed_ms(stats_start_us, now_us);
            const uint32_t fps = window_ms > 0 ? (frames_in_window * 1000) / window_ms : 0;
            ESP_LOGI(TAG, "frames=%lu fps=%lu max input/update/render=%lu/%lu/%lums",
                     (unsigned long)frames_in_window,
                     (unsigned long)fps,
                     (unsigned long)max_input_ms,
                     (unsigned long)max_update_ms,
                     (unsigned long)max_render_ms);
            stats_start_us = now_us;
            frames_in_window = 0;
            max_input_ms = 0;
            max_update_ms = 0;
            max_render_ms = 0;
        }

        if (config->stop_requested != NULL && *config->stop_requested) {
            return ESP_OK;
        }

        next_frame_us += frame_period_us;
        if (esp_timer_get_time() < next_frame_us) {
            wait_until_us(next_frame_us);
        } else {
            next_frame_us = esp_timer_get_time();
        }
    }

    return ESP_OK;
}
