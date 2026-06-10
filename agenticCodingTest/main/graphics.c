#include "graphics.h"

#include <ctype.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_check.h"
#include "font_5x7.h"

static const uint8_t *glyph_for_char(char ch)
{
    if (ch >= '0' && ch <= '9') {
        return FONT_5X7_DIGITS[ch - '0'];
    }

    ch = (char)toupper((unsigned char)ch);
    if (ch >= 'A' && ch <= 'Z') {
        return FONT_5X7_LETTERS[ch - 'A'];
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
            for (uint16_t col = 0; col < FONT_5X7_WIDTH; ++col) {
                for (uint16_t row = 0; row < FONT_5X7_HEIGHT; ++row) {
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
        cursor_x += (uint16_t)((FONT_5X7_WIDTH + FONT_5X7_SPACING) * scale);
    }

    return ESP_OK;
}

esp_err_t graphics_draw_sprite(graphics_t *graphics,
                               int32_t x,
                               int32_t y,
                               uint16_t width,
                               uint16_t height,
                               const uint16_t *pixels,
                               uint16_t transparent_key,
                               uint16_t screen_width,
                               uint16_t screen_height)
{
    ESP_RETURN_ON_FALSE(graphics != NULL && graphics->display != NULL, ESP_ERR_INVALID_ARG, "graphics", "not initialized");
    ESP_RETURN_ON_FALSE(pixels != NULL, ESP_ERR_INVALID_ARG, "graphics", "pixels required");

    for (uint16_t row = 0; row < height; row++) {
        int32_t py = y + (int32_t)row;
        if (py < 0 || py >= (int32_t)screen_height) {
            continue;
        }

        uint16_t col = 0;
        const uint16_t *row_pixels = &pixels[(uint32_t)row * width];

        while (col < width) {
            while (col < width && row_pixels[col] == transparent_key) {
                col++;
            }
            if (col >= width) {
                break;
            }

            uint16_t run_start = col;
            while (col < width && row_pixels[col] != transparent_key) {
                col++;
            }
            uint16_t run_len = col - run_start;

            int32_t px = x + (int32_t)run_start;
            int32_t clip_x = px;
            int32_t clip_w = (int32_t)run_len;
            int32_t pixel_offset = 0;

            if (clip_x < 0) {
                pixel_offset = -clip_x;
                clip_w += clip_x;
                clip_x = 0;
            }
            if (clip_x >= (int32_t)screen_width || clip_w <= 0) {
                continue;
            }
            if (clip_x + clip_w > (int32_t)screen_width) {
                clip_w = (int32_t)screen_width - clip_x;
            }

            ESP_RETURN_ON_ERROR(
                st7789_display_push_pixels(graphics->display,
                                           (uint16_t)clip_x, (uint16_t)py,
                                           (uint16_t)clip_w, 1,
                                           &row_pixels[run_start + (uint16_t)pixel_offset],
                                           (size_t)clip_w),
                "graphics", "sprite run push failed");
        }
    }

    return ESP_OK;
}

esp_err_t graphics_draw_sprite_scaled(graphics_t *graphics,
                                      int32_t x,
                                      int32_t y,
                                      uint16_t width,
                                      uint16_t height,
                                      const uint16_t *pixels,
                                      uint16_t transparent_key,
                                      uint16_t screen_width,
                                      uint16_t screen_height,
                                      uint8_t scale)
{
    ESP_RETURN_ON_FALSE(graphics != NULL && graphics->display != NULL, ESP_ERR_INVALID_ARG, "graphics", "not initialized");
    ESP_RETURN_ON_FALSE(pixels != NULL, ESP_ERR_INVALID_ARG, "graphics", "pixels required");
    ESP_RETURN_ON_FALSE(scale > 0, ESP_ERR_INVALID_ARG, "graphics", "scale must be positive");

    if (scale == 1) {
        return graphics_draw_sprite(graphics, x, y, width, height, pixels, transparent_key, screen_width, screen_height);
    }

    for (uint16_t row = 0; row < height; row++) {
        const uint16_t *rp = &pixels[(uint32_t)row * width];

        for (uint8_t s = 0; s < scale; s++) {
            int32_t py = y + (int32_t)row * scale + (int32_t)s;
            if (py < 0 || py >= (int32_t)screen_height) {
                continue;
            }

            uint16_t col = 0;
            while (col < width) {
                while (col < width && rp[col] == transparent_key) {
                    col++;
                }
                if (col >= width) {
                    break;
                }

                uint16_t run_start = col;
                uint16_t run_color = rp[col];
                while (col < width && rp[col] != transparent_key && rp[col] == run_color) {
                    col++;
                }
                uint16_t run_len = col - run_start;

                int32_t px = x + (int32_t)run_start * scale;
                int32_t pw = (int32_t)run_len * scale;

                if (px + pw <= 0 || px >= (int32_t)screen_width) {
                    continue;
                }
                int32_t cx = px < 0 ? 0 : px;
                int32_t cw = pw - (cx - px);
                if (cx + cw > (int32_t)screen_width) {
                    cw = (int32_t)screen_width - cx;
                }
                if (cw <= 0) {
                    continue;
                }

                ESP_RETURN_ON_ERROR(
                    st7789_display_draw_rect(graphics->display, (uint16_t)cx, (uint16_t)py, (uint16_t)cw, 1, run_color),
                    "graphics", "scaled sprite rect failed");
            }
        }
    }

    return ESP_OK;
}
