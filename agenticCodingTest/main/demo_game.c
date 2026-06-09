#include "demo_game.h"

#include <stddef.h>
#include <stdint.h>

#include "board_config.h"
#include "esp_check.h"

#define COLOR_BACKGROUND 0x0000
#define COLOR_BORDER 0x39e7
#define COLOR_TEXT 0xffff
#define COLOR_PADDLE 0x07e0
#define COLOR_ITEM 0xf81f
#define COLOR_MISS 0xf800

#define HUD_HEIGHT 22
#define PADDLE_WIDTH 30
#define PADDLE_HEIGHT 6
#define ITEM_SIZE 8
#define PADDLE_SPEED_PX_PER_SEC 105
#define BASE_FALL_SPEED_PX_PER_SEC 58

static uint32_t next_random(demo_game_t *game)
{
    game->rng = (game->rng * 1664525u) + 1013904223u;
    return game->rng;
}

static int32_t clamp_i32(int32_t value, int32_t minimum, int32_t maximum)
{
    if (value < minimum) {
        return minimum;
    }
    if (value > maximum) {
        return maximum;
    }
    return value;
}

static bool overlaps(int32_t ax, int32_t ay, int32_t aw, int32_t ah, int32_t bx, int32_t by, int32_t bw, int32_t bh)
{
    return ax < bx + bw && ax + aw > bx && ay < by + bh && ay + ah > by;
}

static void spawn_item(demo_game_t *game)
{
    const board_config_t *board = board_config_get();
    const uint32_t range = board->display_width - ITEM_SIZE - 4;

    game->item_x = 2 + (int32_t)(next_random(game) % range);
    game->item_y_milli = HUD_HEIGHT * 1000;
}

static esp_err_t draw_score(demo_game_t *game)
{
    ESP_RETURN_ON_ERROR(graphics_fill_rect(game->graphics, 1, 1, 96, HUD_HEIGHT - 2, COLOR_BACKGROUND),
                        "demo_game",
                        "score clear failed");
    ESP_RETURN_ON_ERROR(graphics_draw_text(game->graphics, 4, 5, "SCORE", COLOR_TEXT, 1),
                        "demo_game",
                        "score label failed");

    char score_text[11];
    uint32_t value = game->score;
    int index = 0;
    if (value == 0) {
        score_text[index++] = '0';
    } else {
        char reversed[10];
        int reversed_count = 0;
        while (value > 0 && reversed_count < (int)sizeof(reversed)) {
            reversed[reversed_count++] = (char)('0' + (value % 10));
            value /= 10;
        }
        while (reversed_count > 0) {
            score_text[index++] = reversed[--reversed_count];
        }
    }
    score_text[index] = '\0';

    return graphics_draw_text(game->graphics, 44, 5, score_text, COLOR_TEXT, 1);
}

esp_err_t demo_game_init(demo_game_t *game, graphics_t *graphics, bool left_pressed, bool right_pressed)
{
    ESP_RETURN_ON_FALSE(game != NULL, ESP_ERR_INVALID_ARG, "demo_game", "game is required");
    ESP_RETURN_ON_FALSE(graphics != NULL, ESP_ERR_INVALID_ARG, "demo_game", "graphics is required");

    const board_config_t *board = board_config_get();
    *game = (demo_game_t) {
        .graphics = graphics,
        .left_pressed = left_pressed,
        .right_pressed = right_pressed,
        .paddle_x = (board->display_width - PADDLE_WIDTH) / 2,
        .previous_paddle_x = -1,
        .previous_item_x = -1,
        .previous_item_y = -1,
        .previous_score = UINT32_MAX,
        .rng = 0x88bfca74u,
        .redraw_all = true,
    };
    spawn_item(game);

    ESP_RETURN_ON_ERROR(graphics_clear(graphics, COLOR_BACKGROUND), "demo_game", "screen clear failed");
    ESP_RETURN_ON_ERROR(graphics_draw_rect(graphics, 0, 0, board->display_width, board->display_height, COLOR_BORDER),
                        "demo_game",
                        "border failed");
    return graphics_draw_text(graphics, 91, 5, "CATCH", COLOR_TEXT, 1);
}

esp_err_t demo_game_handle_input(demo_game_t *game, const button_event_t *events, size_t event_count)
{
    ESP_RETURN_ON_FALSE(game != NULL, ESP_ERR_INVALID_ARG, "demo_game", "game is required");
    ESP_RETURN_ON_FALSE(events != NULL || event_count == 0, ESP_ERR_INVALID_ARG, "demo_game", "events are required");

    for (size_t i = 0; i < event_count; ++i) {
        const bool pressed = events[i].type == BUTTON_EVENT_PRESSED;
        if (events[i].id == BUTTON_ID_LEFT) {
            game->left_pressed = pressed;
        } else if (events[i].id == BUTTON_ID_RIGHT) {
            game->right_pressed = pressed;
        }
    }

    return ESP_OK;
}

esp_err_t demo_game_update(demo_game_t *game, uint32_t frame, uint32_t dt_ms)
{
    ESP_RETURN_ON_FALSE(game != NULL, ESP_ERR_INVALID_ARG, "demo_game", "game is required");
    (void)frame;

    const board_config_t *board = board_config_get();
    const int32_t paddle_y = board->display_height - 15;
    const int32_t move_step = (PADDLE_SPEED_PX_PER_SEC * (int32_t)dt_ms) / 1000;
    const int32_t fall_speed = BASE_FALL_SPEED_PX_PER_SEC + (int32_t)(game->score * 3);
    const int32_t fall_step_milli = fall_speed * (int32_t)dt_ms;

    if (game->left_pressed && !game->right_pressed) {
        game->paddle_x -= move_step;
    } else if (game->right_pressed && !game->left_pressed) {
        game->paddle_x += move_step;
    }
    game->paddle_x = clamp_i32(game->paddle_x, 2, board->display_width - PADDLE_WIDTH - 2);

    game->item_y_milli += fall_step_milli;
    const int32_t item_y = game->item_y_milli / 1000;
    if (overlaps(game->item_x, item_y, ITEM_SIZE, ITEM_SIZE, game->paddle_x, paddle_y, PADDLE_WIDTH, PADDLE_HEIGHT)) {
        ++game->score;
        spawn_item(game);
    } else if (item_y > board->display_height - ITEM_SIZE - 1) {
        game->score = 0;
        spawn_item(game);
    }

    return ESP_OK;
}

esp_err_t demo_game_render(demo_game_t *game)
{
    ESP_RETURN_ON_FALSE(game != NULL, ESP_ERR_INVALID_ARG, "demo_game", "game is required");

    const board_config_t *board = board_config_get();
    const int32_t paddle_y = board->display_height - 15;
    const int32_t item_y = game->item_y_milli / 1000;

    if (game->redraw_all || game->score != game->previous_score) {
        ESP_RETURN_ON_ERROR(draw_score(game), "demo_game", "score render failed");
        game->previous_score = game->score;
    }

    if (game->previous_item_x >= 0) {
        ESP_RETURN_ON_ERROR(graphics_fill_rect(game->graphics,
                                               (uint16_t)game->previous_item_x,
                                               (uint16_t)game->previous_item_y,
                                               ITEM_SIZE,
                                               ITEM_SIZE,
                                               COLOR_BACKGROUND),
                            "demo_game",
                            "old item clear failed");
    }
    ESP_RETURN_ON_ERROR(graphics_fill_rect(game->graphics,
                                           (uint16_t)game->item_x,
                                           (uint16_t)item_y,
                                           ITEM_SIZE,
                                           ITEM_SIZE,
                                           game->score == 0 ? COLOR_MISS : COLOR_ITEM),
                        "demo_game",
                        "item render failed");

    if (game->redraw_all || game->previous_paddle_x != game->paddle_x) {
        if (game->previous_paddle_x >= 0) {
            ESP_RETURN_ON_ERROR(graphics_fill_rect(game->graphics,
                                                   (uint16_t)game->previous_paddle_x,
                                                   (uint16_t)paddle_y,
                                                   PADDLE_WIDTH,
                                                   PADDLE_HEIGHT,
                                                   COLOR_BACKGROUND),
                                "demo_game",
                                "old paddle clear failed");
        }
        ESP_RETURN_ON_ERROR(graphics_fill_rect(game->graphics,
                                               (uint16_t)game->paddle_x,
                                               (uint16_t)paddle_y,
                                               PADDLE_WIDTH,
                                               PADDLE_HEIGHT,
                                               COLOR_PADDLE),
                            "demo_game",
                            "paddle render failed");
    }

    game->previous_item_x = game->item_x;
    game->previous_item_y = item_y;
    game->previous_paddle_x = game->paddle_x;
    game->redraw_all = false;
    return ESP_OK;
}
