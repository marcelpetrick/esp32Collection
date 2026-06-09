#include "graphics.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_check.h"

#define FONT_WIDTH 5
#define FONT_HEIGHT 7
#define FONT_SPACING 1

static const uint8_t DIGIT_FONT[10][FONT_WIDTH] = {
    {0x3e, 0x51, 0x49, 0x45, 0x3e},
    {0x00, 0x42, 0x7f, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4b, 0x31},
    {0x18, 0x14, 0x12, 0x7f, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3c, 0x4a, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1e},
};

static const uint8_t LETTER_FONT[26][FONT_WIDTH] = {
    {0x7e, 0x11, 0x11, 0x11, 0x7e},
    {0x7f, 0x49, 0x49, 0x49, 0x36},
    {0x3e, 0x41, 0x41, 0x41, 0x22},
    {0x7f, 0x41, 0x41, 0x22, 0x1c},
    {0x7f, 0x49, 0x49, 0x49, 0x41},
    {0x7f, 0x09, 0x09, 0x09, 0x01},
    {0x3e, 0x41, 0x49, 0x49, 0x7a},
    {0x7f, 0x08, 0x08, 0x08, 0x7f},
    {0x00, 0x41, 0x7f, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3f, 0x01},
    {0x7f, 0x08, 0x14, 0x22, 0x41},
    {0x7f, 0x40, 0x40, 0x40, 0x40},
    {0x7f, 0x02, 0x0c, 0x02, 0x7f},
    {0x7f, 0x04, 0x08, 0x10, 0x7f},
    {0x3e, 0x41, 0x41, 0x41, 0x3e},
    {0x7f, 0x09, 0x09, 0x09, 0x06},
    {0x3e, 0x41, 0x51, 0x21, 0x5e},
    {0x7f, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7f, 0x01, 0x01},
    {0x3f, 0x40, 0x40, 0x40, 0x3f},
    {0x1f, 0x20, 0x40, 0x20, 0x1f},
    {0x3f, 0x40, 0x38, 0x40, 0x3f},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43},
};

static const uint8_t *glyph_for_char(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return DIGIT_FONT[ch - '0'];
    }

    ch = (char)toupper((unsigned char)ch);
    if (ch >= 'A' && ch <= 'Z') {
        return LETTER_FONT[ch - 'A'];
    }

    return NULL;
}

esp_err_t graphics_init(graphics_t *graphics, st7789_display_t *display)
{
    ESP_RETURN_ON_FALSE(graphics != NULL, ESP_ERR_INVALID_ARG, "graphics", "graphics is required");
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, "graphics", "display is required");

    graphics->display = display;
    return ESP_OK;
}

esp_err_t graphics_clear(graphics_t *graphics, uint16_t color565)
{
    ESP_RETURN_ON_FALSE(graphics != NULL && graphics->display != NULL, ESP_ERR_INVALID_ARG, "graphics", "graphics is not initialized");
    return st7789_display_fill(graphics->display, color565);
}

esp_err_t graphics_fill_rect(graphics_t *graphics,
                             uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height,
                             uint16_t color565)
{
    ESP_RETURN_ON_FALSE(graphics != NULL && graphics->display != NULL, ESP_ERR_INVALID_ARG, "graphics", "graphics is not initialized");
    return st7789_display_draw_rect(graphics->display, x, y, width, height, color565);
}

esp_err_t graphics_draw_rect(graphics_t *graphics,
                             uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height,
                             uint16_t color565)
{
    ESP_RETURN_ON_FALSE(width > 0 && height > 0, ESP_ERR_INVALID_ARG, "graphics", "invalid rectangle");
    ESP_RETURN_ON_ERROR(graphics_fill_rect(graphics, x, y, width, 1, color565), "graphics", "top edge failed");
    if (height > 1) {
        ESP_RETURN_ON_ERROR(graphics_fill_rect(graphics, x, y + height - 1, width, 1, color565), "graphics", "bottom edge failed");
    }
    if (height > 2) {
        ESP_RETURN_ON_ERROR(graphics_fill_rect(graphics, x, y + 1, 1, height - 2, color565), "graphics", "left edge failed");
        if (width > 1) {
            ESP_RETURN_ON_ERROR(graphics_fill_rect(graphics, x + width - 1, y + 1, 1, height - 2, color565),
                                "graphics",
                                "right edge failed");
        }
    }

    return ESP_OK;
}

esp_err_t graphics_draw_bitmap(graphics_t *graphics,
                               uint16_t x,
                               uint16_t y,
                               uint16_t width,
                               uint16_t height,
                               const uint16_t *pixels,
                               size_t pixel_count)
{
    ESP_RETURN_ON_FALSE(graphics != NULL && graphics->display != NULL, ESP_ERR_INVALID_ARG, "graphics", "graphics is not initialized");
    return st7789_display_push_pixels(graphics->display, x, y, width, height, pixels, pixel_count);
}

esp_err_t graphics_draw_text(graphics_t *graphics,
                             uint16_t x,
                             uint16_t y,
                             const char *text,
                             uint16_t color565,
                             uint16_t scale)
{
    ESP_RETURN_ON_FALSE(graphics != NULL && graphics->display != NULL, ESP_ERR_INVALID_ARG, "graphics", "graphics is not initialized");
    ESP_RETURN_ON_FALSE(text != NULL, ESP_ERR_INVALID_ARG, "graphics", "text is required");
    ESP_RETURN_ON_FALSE(scale > 0, ESP_ERR_INVALID_ARG, "graphics", "scale must be positive");

    uint16_t cursor_x = x;
    for (const char *cursor = text; *cursor != '\0'; ++cursor) {
        const uint8_t *glyph = glyph_for_char(*cursor);
        if (glyph != NULL) {
            for (uint16_t col = 0; col < FONT_WIDTH; ++col) {
                for (uint16_t row = 0; row < FONT_HEIGHT; ++row) {
                    if ((glyph[col] & (1u << row)) != 0) {
                        ESP_RETURN_ON_ERROR(graphics_fill_rect(graphics,
                                                               cursor_x + (col * scale),
                                                               y + (row * scale),
                                                               scale,
                                                               scale,
                                                               color565),
                                            "graphics",
                                            "glyph pixel failed");
                    }
                }
            }
        }
        cursor_x += (FONT_WIDTH + FONT_SPACING) * scale;
    }

    return ESP_OK;
}
