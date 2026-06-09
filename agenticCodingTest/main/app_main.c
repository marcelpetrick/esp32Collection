#include "board_config.h"
#include "button_input.h"
#include "display_smoke_test.h"
#include "st7789_display.h"

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "tdisplay_games";

static esp_err_t draw_button_feedback(st7789_display_t *display, button_id_t id, bool pressed)
{
    const board_config_t *board = board_config_get();
    const uint16_t width = 28;
    const uint16_t height = 12;
    const uint16_t x = id == BUTTON_ID_LEFT ? 8 : board->display_width - width - 8;
    const uint16_t y = board->display_height - height - 8;
    const uint16_t color = pressed ? 0x07e0 : 0x2104;

    return st7789_display_draw_rect(display, x, y, width, height, color);
}

void app_main(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;
    int chip_rev_major = 0;
    int chip_rev_minor = 0;
    st7789_display_t *display = NULL;
    button_input_t buttons = {0};

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
    ESP_ERROR_CHECK(draw_button_feedback(display, BUTTON_ID_LEFT, button_input_is_pressed(&buttons, BUTTON_ID_LEFT)));
    ESP_ERROR_CHECK(draw_button_feedback(display, BUTTON_ID_RIGHT, button_input_is_pressed(&buttons, BUTTON_ID_RIGHT)));

    uint32_t tick = 0;
    TickType_t last_heartbeat = xTaskGetTickCount() - pdMS_TO_TICKS(5000);
    while (true) {
        button_event_t events[BUTTON_ID_COUNT];
        size_t event_count = 0;

        ESP_ERROR_CHECK(button_input_poll(&buttons, events, BUTTON_ID_COUNT, &event_count));
        for (size_t i = 0; i < event_count; ++i) {
            const bool pressed = events[i].type == BUTTON_EVENT_PRESSED;
            ESP_LOGI(TAG, "button %s %s at %llums",
                     button_input_name(events[i].id),
                     pressed ? "pressed" : "released",
                     (unsigned long long)events[i].timestamp_ms);
            ESP_ERROR_CHECK(draw_button_feedback(display, events[i].id, pressed));
        }

        const TickType_t now = xTaskGetTickCount();
        if (now - last_heartbeat >= pdMS_TO_TICKS(5000)) {
            ESP_LOGI(TAG, "heartbeat %lu", (unsigned long)tick++);
            last_heartbeat = now;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
