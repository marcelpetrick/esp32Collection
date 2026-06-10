#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef struct st7789_display st7789_display_t;

esp_err_t st7789_display_init(st7789_display_t **display_out);
void st7789_display_destroy(st7789_display_t *display);
esp_err_t st7789_display_set_backlight(st7789_display_t *display, bool enabled);
esp_err_t st7789_display_fill(st7789_display_t *display, uint16_t color565);
/* High fixed overhead (3 SPI setup transactions per call); use push_pixels for multi-pixel writes. */
esp_err_t st7789_display_draw_pixel(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t color565);
esp_err_t st7789_display_draw_rect(st7789_display_t *display,
                                   uint16_t x,
                                   uint16_t y,
                                   uint16_t width,
                                   uint16_t height,
                                   uint16_t color565);
esp_err_t st7789_display_push_pixels(st7789_display_t *display,
                                     uint16_t x,
                                     uint16_t y,
                                     uint16_t width,
                                     uint16_t height,
                                     const uint16_t *pixels,
                                     size_t pixel_count);
