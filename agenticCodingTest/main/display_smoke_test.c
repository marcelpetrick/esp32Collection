#include "display_smoke_test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "board_config.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xffff
#define COLOR_RED 0xf800
#define COLOR_GREEN 0x07e0
#define COLOR_BLUE 0x001f
#define COLOR_YELLOW 0xffe0
#define COLOR_CYAN 0x07ff
#define COLOR_MAGENTA 0xf81f

#define CHECKER_WIDTH 32
#define CHECKER_HEIGHT 32

static const char *TAG = "display_smoke";
static uint16_t checker_pixels[CHECKER_WIDTH * CHECKER_HEIGHT];

static void pause_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static esp_err_t fill_phase(st7789_display_t *display, const char *name, uint16_t color)
{
    ESP_LOGI(TAG, "fill %s", name);
    ESP_RETURN_ON_ERROR(st7789_display_fill(display, color), TAG, "fill %s failed", name);
    pause_ms(450);
    return ESP_OK;
}

static esp_err_t draw_border(st7789_display_t *display,
                             uint16_t width,
                             uint16_t height,
                             uint16_t color,
                             uint16_t thickness)
{
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 0, 0, width, thickness, color), TAG, "top border failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 0, height - thickness, width, thickness, color), TAG, "bottom border failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 0, 0, thickness, height, color), TAG, "left border failed");
    return st7789_display_draw_rect(display, width - thickness, 0, thickness, height, color);
}

static esp_err_t draw_grid(st7789_display_t *display, uint16_t width, uint16_t height)
{
    const uint16_t x_step = width / 5;
    const uint16_t y_step = height / 6;

    for (uint16_t x = x_step; x < width; x += x_step) {
        ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, x, 0, 1, height, 0x39e7), TAG, "vertical grid failed");
    }
    for (uint16_t y = y_step; y < height; y += y_step) {
        ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 0, y, width, 1, 0x39e7), TAG, "horizontal grid failed");
    }

    return ESP_OK;
}

static esp_err_t draw_checker(st7789_display_t *display, uint16_t x, uint16_t y)
{
    for (uint16_t row = 0; row < CHECKER_HEIGHT; ++row) {
        for (uint16_t col = 0; col < CHECKER_WIDTH; ++col) {
            const bool left = col < CHECKER_WIDTH / 2;
            const bool top = row < CHECKER_HEIGHT / 2;
            checker_pixels[(row * CHECKER_WIDTH) + col] = (left == top) ? COLOR_YELLOW : COLOR_BLUE;
        }
    }

    return st7789_display_push_pixels(display, x, y, CHECKER_WIDTH, CHECKER_HEIGHT, checker_pixels, CHECKER_WIDTH * CHECKER_HEIGHT);
}

static esp_err_t draw_diagonal_pixels(st7789_display_t *display, uint16_t width, uint16_t height)
{
    const uint16_t limit = width < height ? width : height;
    for (uint16_t i = 0; i < limit; ++i) {
        ESP_RETURN_ON_ERROR(st7789_display_draw_pixel(display, i, i, COLOR_WHITE), TAG, "primary diagonal failed");
        ESP_RETURN_ON_ERROR(st7789_display_draw_pixel(display, width - i - 1, i, COLOR_CYAN), TAG, "secondary diagonal failed");
    }

    return ESP_OK;
}

static esp_err_t draw_block_o(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t scale, uint16_t color)
{
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, x, y, 5 * scale, scale, color), TAG, "O top failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, x, y + (6 * scale), 5 * scale, scale, color), TAG, "O bottom failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, x, y, scale, 7 * scale, color), TAG, "O left failed");
    return st7789_display_draw_rect(display, x + (4 * scale), y, scale, 7 * scale, color);
}

static esp_err_t draw_block_k(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t scale, uint16_t color)
{
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, x, y, scale, 7 * scale, color), TAG, "K stem failed");
    for (uint16_t step = 0; step < 4; ++step) {
        ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display,
                                                     x + ((1 + step) * scale),
                                                     y + ((3 - step) * scale),
                                                     scale,
                                                     scale,
                                                     color),
                            TAG,
                            "K upper failed");
        ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display,
                                                     x + ((1 + step) * scale),
                                                     y + ((3 + step) * scale),
                                                     scale,
                                                     scale,
                                                     color),
                            TAG,
                            "K lower failed");
    }

    return ESP_OK;
}

static esp_err_t draw_final_pattern(st7789_display_t *display)
{
    const board_config_t *board = board_config_get();
    const uint16_t width = board->display_width;
    const uint16_t height = board->display_height;

    ESP_LOGI(TAG, "draw final pattern");
    ESP_RETURN_ON_ERROR(st7789_display_fill(display, COLOR_BLACK), TAG, "clear final pattern failed");
    ESP_RETURN_ON_ERROR(draw_border(display, width, height, COLOR_WHITE, 2), TAG, "border failed");
    ESP_RETURN_ON_ERROR(draw_grid(display, width, height), TAG, "grid failed");

    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 8, 8, 24, 24, COLOR_RED), TAG, "red swatch failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 36, 8, 24, 24, COLOR_GREEN), TAG, "green swatch failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 64, 8, 24, 24, COLOR_BLUE), TAG, "blue swatch failed");
    ESP_RETURN_ON_ERROR(st7789_display_draw_rect(display, 92, 8, 24, 24, COLOR_WHITE), TAG, "white swatch failed");

    ESP_RETURN_ON_ERROR(draw_checker(display, 8, 48), TAG, "checker failed");
    ESP_RETURN_ON_ERROR(draw_diagonal_pixels(display, width, height), TAG, "diagonal pixels failed");
    ESP_RETURN_ON_ERROR(draw_block_o(display, 46, 148, 5, COLOR_MAGENTA), TAG, "block O failed");
    ESP_RETURN_ON_ERROR(draw_block_k(display, 78, 148, 5, COLOR_CYAN), TAG, "block K failed");

    return ESP_OK;
}

esp_err_t display_smoke_test_run(st7789_display_t *display)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is required");

    ESP_LOGI(TAG, "start display smoke test");
    ESP_RETURN_ON_ERROR(fill_phase(display, "red", COLOR_RED), TAG, "red phase failed");
    ESP_RETURN_ON_ERROR(fill_phase(display, "green", COLOR_GREEN), TAG, "green phase failed");
    ESP_RETURN_ON_ERROR(fill_phase(display, "blue", COLOR_BLUE), TAG, "blue phase failed");
    ESP_RETURN_ON_ERROR(fill_phase(display, "white", COLOR_WHITE), TAG, "white phase failed");
    ESP_RETURN_ON_ERROR(fill_phase(display, "black", COLOR_BLACK), TAG, "black phase failed");
    ESP_RETURN_ON_ERROR(draw_final_pattern(display), TAG, "final pattern failed");
    ESP_LOGI(TAG, "display smoke test complete");

    return ESP_OK;
}
