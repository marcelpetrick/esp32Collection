#pragma once

#include <stddef.h>
#include <stdint.h>

#include "button_input.h"
#include "esp_err.h"
#include "st7789_display.h"

typedef esp_err_t (*game_loop_input_cb_t)(const button_event_t *events, size_t event_count, void *ctx);
typedef esp_err_t (*game_loop_step_cb_t)(uint32_t frame, uint32_t dt_ms, void *ctx);

typedef struct {
    uint32_t target_fps;
    uint32_t stats_interval_ms;
    void *ctx;
    game_loop_input_cb_t on_input;
    game_loop_step_cb_t on_update;
    game_loop_step_cb_t on_render;
} game_loop_config_t;

esp_err_t game_loop_run(st7789_display_t *display, button_input_t *buttons, const game_loop_config_t *config);
