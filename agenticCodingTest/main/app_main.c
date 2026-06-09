#include "board_config.h"
#include "button_input.h"
#include "demo_game.h"
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
    demo_game_t game;
} app_context_t;

static esp_err_t handle_input(const button_event_t *events, size_t event_count, void *ctx)
{
    app_context_t *app = (app_context_t *)ctx;

    for (size_t i = 0; i < event_count; ++i) {
        const bool pressed = events[i].type == BUTTON_EVENT_PRESSED;
        ESP_LOGI(TAG, "button %s %s at %llums",
                 button_input_name(events[i].id),
                 pressed ? "pressed" : "released",
                 (unsigned long long)events[i].timestamp_ms);
    }

    return demo_game_handle_input(&app->game, events, event_count);
}

static esp_err_t update_game(uint32_t frame, uint32_t dt_ms, void *ctx)
{
    app_context_t *app = (app_context_t *)ctx;
    return demo_game_update(&app->game, frame, dt_ms);
}

static esp_err_t render_game(uint32_t frame, uint32_t dt_ms, void *ctx)
{
    (void)frame;
    (void)dt_ms;
    app_context_t *app = (app_context_t *)ctx;

    return demo_game_render(&app->game);
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
    ESP_ERROR_CHECK(demo_game_init(&app.game,
                                   &app.graphics,
                                   button_input_is_pressed(&buttons, BUTTON_ID_LEFT),
                                   button_input_is_pressed(&buttons, BUTTON_ID_RIGHT)));

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
