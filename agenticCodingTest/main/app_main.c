#include "board_config.h"
#include "button_input.h"
#include "display_smoke_test.h"
#include "game_loop.h"
#include "graphics.h"
#include "st7789_display.h"

#include "esp_chip_info.h"
#include "esp_check.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "tdisplay_games";

typedef struct {
    graphics_t graphics;
    bool button_pressed[BUTTON_ID_COUNT];
    bool render_dirty;
} app_context_t;

static esp_err_t draw_button_feedback(graphics_t *graphics, button_id_t id, bool pressed)
{
    const board_config_t *board = board_config_get();
    const uint16_t width = 28;
    const uint16_t height = 12;
    const uint16_t x = id == BUTTON_ID_LEFT ? 8 : board->display_width - width - 8;
    const uint16_t y = board->display_height - height - 8;
    const uint16_t color = pressed ? 0x07e0 : 0x2104;
    const uint16_t label_x = x + 9;
    const char *label = id == BUTTON_ID_LEFT ? "L" : "R";

    ESP_RETURN_ON_ERROR(graphics_fill_rect(graphics, x, y, width, height, color), TAG, "button fill failed");
    return graphics_draw_text(graphics, label_x, y + 2, label, 0xffff, 1);
}

static esp_err_t handle_input(const button_event_t *events, size_t event_count, void *ctx)
{
    app_context_t *app = (app_context_t *)ctx;

    for (size_t i = 0; i < event_count; ++i) {
        const bool pressed = events[i].type == BUTTON_EVENT_PRESSED;
        app->button_pressed[events[i].id] = pressed;
        app->render_dirty = true;
        ESP_LOGI(TAG, "button %s %s at %llums",
                 button_input_name(events[i].id),
                 pressed ? "pressed" : "released",
                 (unsigned long long)events[i].timestamp_ms);
    }

    return ESP_OK;
}

static esp_err_t update_game(uint32_t frame, uint32_t dt_ms, void *ctx)
{
    (void)frame;
    (void)dt_ms;
    (void)ctx;
    return ESP_OK;
}

static esp_err_t render_game(uint32_t frame, uint32_t dt_ms, void *ctx)
{
    (void)frame;
    (void)dt_ms;
    app_context_t *app = (app_context_t *)ctx;

    if (!app->render_dirty) {
        return ESP_OK;
    }

    ESP_RETURN_ON_ERROR(draw_button_feedback(&app->graphics, BUTTON_ID_LEFT, app->button_pressed[BUTTON_ID_LEFT]),
                        TAG,
                        "left button render failed");
    ESP_RETURN_ON_ERROR(draw_button_feedback(&app->graphics, BUTTON_ID_RIGHT, app->button_pressed[BUTTON_ID_RIGHT]),
                        TAG,
                        "right button render failed");
    app->render_dirty = false;
    return ESP_OK;
}

void app_main(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;
    int chip_rev_major = 0;
    int chip_rev_minor = 0;
    st7789_display_t *display = NULL;
    button_input_t buttons = {0};
    app_context_t app = {0};

    esp_chip_info(&chip_info);
    esp_flash_get_size(NULL, &flash_size);
    chip_rev_major = chip_info.revision / 100;
    chip_rev_minor = chip_info.revision % 100;

    ESP_LOGI(TAG, "TENSTAR/T-Display ESP32 game firmware booting");
    ESP_LOGI(TAG, "Chip cores: %d, revision: v%d.%d", chip_info.cores, chip_rev_major, chip_rev_minor);
    ESP_LOGI(TAG, "Features: WiFi=%s BT=%s BLE=%s",
             (chip_info.features & CHIP_FEATURE_WIFI_BGN) ? "yes" : "no",
             (chip_info.features & CHIP_FEATURE_BT) ? "yes" : "no",
             (chip_info.features & CHIP_FEATURE_BLE) ? "yes" : "no");
    ESP_LOGI(TAG, "Flash size: %lu bytes", (unsigned long)flash_size);
    board_config_log();
    ESP_ERROR_CHECK(st7789_display_init(&display));
    ESP_ERROR_CHECK(st7789_display_fill(display, 0x0000));
    ESP_LOGI(TAG, "Display driver initialized and cleared");
    ESP_ERROR_CHECK(display_smoke_test_run(display));
    ESP_ERROR_CHECK(button_input_init(&buttons, 30));

    ESP_ERROR_CHECK(graphics_init(&app.graphics, display));
    app.button_pressed[BUTTON_ID_LEFT] = button_input_is_pressed(&buttons, BUTTON_ID_LEFT);
    app.button_pressed[BUTTON_ID_RIGHT] = button_input_is_pressed(&buttons, BUTTON_ID_RIGHT);
    app.render_dirty = true;

    const game_loop_config_t loop_config = {
        .target_fps = 30,
        .stats_interval_ms = 5000,
        .ctx = &app,
        .on_input = handle_input,
        .on_update = update_game,
        .on_render = render_game,
    };
    ESP_ERROR_CHECK(game_loop_run(display, &buttons, &loop_config));
}
