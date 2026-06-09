#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "button_input.h"
#include "esp_err.h"
#include "graphics.h"

typedef struct {
    graphics_t *graphics;
    bool left_pressed;
    bool right_pressed;
    int32_t paddle_x;
    int32_t previous_paddle_x;
    int32_t item_x;
    int32_t item_y_milli;
    int32_t previous_item_x;
    int32_t previous_item_y;
    uint32_t score;
    uint32_t previous_score;
    uint32_t rng;
    bool redraw_all;
} demo_game_t;

esp_err_t demo_game_init(demo_game_t *game, graphics_t *graphics, bool left_pressed, bool right_pressed);
esp_err_t demo_game_handle_input(demo_game_t *game, const button_event_t *events, size_t event_count);
esp_err_t demo_game_update(demo_game_t *game, uint32_t frame, uint32_t dt_ms);
esp_err_t demo_game_render(demo_game_t *game);
