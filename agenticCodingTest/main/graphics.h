#pragma once

#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"
#include "st7789_display.h"

typedef struct {
    st7789_display_t *display;
} graphics_t;

esp_err_t graphics_init(graphics_t *graphics, st7789_display_t *display);
esp_err_t graphics_clear(graphics_t *graphics, uint16_t color565);
esp_err_t graphics_fill_rect(graphics_t *graphics,
                             uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height,
                             uint16_t color565);
esp_err_t graphics_draw_rect(graphics_t *graphics,
                             uint16_t x,
                             uint16_t y,
                             uint16_t width,
                             uint16_t height,
                             uint16_t color565);
esp_err_t graphics_draw_bitmap(graphics_t *graphics,
                               uint16_t x,
                               uint16_t y,
                               uint16_t width,
                               uint16_t height,
                               const uint16_t *pixels,
                               size_t pixel_count);
esp_err_t graphics_draw_text(graphics_t *graphics,
                             uint16_t x,
                             uint16_t y,
                             const char *text,
                             uint16_t color565,
                             uint16_t scale);

esp_err_t graphics_draw_sprite(graphics_t *graphics,
                               int32_t x,
                               int32_t y,
                               uint16_t width,
                               uint16_t height,
                               const uint16_t *pixels,
                               uint16_t transparent_key,
                               uint16_t screen_width,
                               uint16_t screen_height);

esp_err_t graphics_draw_sprite_scaled(graphics_t *graphics,
                                      int32_t x,
                                      int32_t y,
                                      uint16_t width,
                                      uint16_t height,
                                      const uint16_t *pixels,
                                      uint16_t transparent_key,
                                      uint16_t screen_width,
                                      uint16_t screen_height,
                                      uint8_t scale);
