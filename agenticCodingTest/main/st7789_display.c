#include "st7789_display.h"

#include <stdlib.h>
#include <string.h>

#include "board_config.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_check.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define ST7789_CMD_SWRESET 0x01
#define ST7789_CMD_SLPOUT 0x11
#define ST7789_CMD_NORON 0x13
#define ST7789_CMD_INVOFF 0x20
#define ST7789_CMD_INVON 0x21
#define ST7789_CMD_DISPON 0x29
#define ST7789_CMD_CASET 0x2A
#define ST7789_CMD_RASET 0x2B
#define ST7789_CMD_RAMWR 0x2C
#define ST7789_CMD_MADCTL 0x36
#define ST7789_CMD_COLMOD 0x3A

#define ST7789_MADCTL_RGB 0x00
#define ST7789_COLMOD_RGB565 0x55
#define ST7789_MAX_CHUNK_PIXELS 512

struct st7789_display {
    const board_config_t *board;
    spi_device_handle_t spi;
    uint16_t scratch[ST7789_MAX_CHUNK_PIXELS];
};

static const char *TAG = "st7789";

static esp_err_t write_bytes(st7789_display_t *display, const uint8_t *data, size_t len, bool is_data)
{
    if (len == 0) {
        return ESP_OK;
    }

    gpio_set_level(display->board->tft_dc, is_data ? 1 : 0);

    spi_transaction_t transaction = {
        .length = len * 8,
        .tx_buffer = data,
    };
    return spi_device_polling_transmit(display->spi, &transaction);
}

static esp_err_t write_command(st7789_display_t *display, uint8_t command)
{
    return write_bytes(display, &command, sizeof(command), false);
}

static esp_err_t write_data(st7789_display_t *display, const uint8_t *data, size_t len)
{
    return write_bytes(display, data, len, true);
}

static esp_err_t write_command_data(st7789_display_t *display, uint8_t command, const uint8_t *data, size_t len)
{
    ESP_RETURN_ON_ERROR(write_command(display, command), TAG, "command 0x%02x failed", command);
    return write_data(display, data, len);
}

static esp_err_t reset_panel(st7789_display_t *display)
{
    gpio_set_level(display->board->tft_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(display->board->tft_rst, 0);
    vTaskDelay(pdMS_TO_TICKS(10));
    gpio_set_level(display->board->tft_rst, 1);
    vTaskDelay(pdMS_TO_TICKS(120));
    return ESP_OK;
}

static esp_err_t set_window(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    const uint16_t x0 = x + display->board->display_x_offset;
    const uint16_t y0 = y + display->board->display_y_offset;
    const uint16_t x1 = x0 + width - 1;
    const uint16_t y1 = y0 + height - 1;
    const uint8_t column_data[] = {
        (uint8_t)(x0 >> 8), (uint8_t)x0,
        (uint8_t)(x1 >> 8), (uint8_t)x1,
    };
    const uint8_t row_data[] = {
        (uint8_t)(y0 >> 8), (uint8_t)y0,
        (uint8_t)(y1 >> 8), (uint8_t)y1,
    };

    ESP_RETURN_ON_ERROR(write_command_data(display, ST7789_CMD_CASET, column_data, sizeof(column_data)), TAG, "set columns failed");
    ESP_RETURN_ON_ERROR(write_command_data(display, ST7789_CMD_RASET, row_data, sizeof(row_data)), TAG, "set rows failed");
    return write_command(display, ST7789_CMD_RAMWR);
}

static bool rect_is_valid(const st7789_display_t *display, uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    return width > 0 &&
           height > 0 &&
           x < display->board->display_width &&
           y < display->board->display_height &&
           width <= display->board->display_width - x &&
           height <= display->board->display_height - y;
}

static esp_err_t init_panel(st7789_display_t *display)
{
    const uint8_t color_mode = ST7789_COLMOD_RGB565;
    const uint8_t madctl = ST7789_MADCTL_RGB;

    ESP_RETURN_ON_ERROR(reset_panel(display), TAG, "reset failed");
    ESP_RETURN_ON_ERROR(write_command(display, ST7789_CMD_SWRESET), TAG, "software reset failed");
    vTaskDelay(pdMS_TO_TICKS(150));
    ESP_RETURN_ON_ERROR(write_command(display, ST7789_CMD_SLPOUT), TAG, "sleep out failed");
    vTaskDelay(pdMS_TO_TICKS(120));
    ESP_RETURN_ON_ERROR(write_command_data(display, ST7789_CMD_COLMOD, &color_mode, sizeof(color_mode)), TAG, "color mode failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(write_command_data(display, ST7789_CMD_MADCTL, &madctl, sizeof(madctl)), TAG, "orientation failed");
    ESP_RETURN_ON_ERROR(write_command(display, ST7789_CMD_INVON), TAG, "inversion failed");
    ESP_RETURN_ON_ERROR(write_command(display, ST7789_CMD_NORON), TAG, "normal mode failed");
    vTaskDelay(pdMS_TO_TICKS(10));
    ESP_RETURN_ON_ERROR(write_command(display, ST7789_CMD_DISPON), TAG, "display on failed");
    vTaskDelay(pdMS_TO_TICKS(100));
    return ESP_OK;
}

esp_err_t st7789_display_init(st7789_display_t **display_out)
{
    ESP_RETURN_ON_FALSE(display_out != NULL, ESP_ERR_INVALID_ARG, TAG, "display_out is required");
    *display_out = NULL;

    const board_config_t *board = board_config_get();
    st7789_display_t *display = calloc(1, sizeof(*display));
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_NO_MEM, TAG, "display allocation failed");
    display->board = board;
    esp_err_t ret = ESP_OK;

    gpio_config_t output_config = {
        .pin_bit_mask = (1ULL << board->tft_dc) | (1ULL << board->tft_rst) | (1ULL << board->tft_bl),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_GOTO_ON_ERROR(gpio_config(&output_config), cleanup, TAG, "gpio output config failed");
    ESP_GOTO_ON_ERROR(st7789_display_set_backlight(display, false), cleanup, TAG, "backlight off failed");

    spi_bus_config_t bus_config = {
        .mosi_io_num = board->tft_mosi,
        .miso_io_num = GPIO_NUM_NC,
        .sclk_io_num = board->tft_sclk,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = ST7789_MAX_CHUNK_PIXELS * sizeof(uint16_t),
    };
    ESP_GOTO_ON_ERROR(spi_bus_initialize(board->spi_host, &bus_config, SPI_DMA_CH_AUTO), cleanup, TAG, "spi bus init failed");

    spi_device_interface_config_t device_config = {
        .clock_speed_hz = (int)board->spi_clock_hz,
        .mode = 0,
        .spics_io_num = board->tft_cs,
        .queue_size = 1,
    };
    ESP_GOTO_ON_ERROR(spi_bus_add_device(board->spi_host, &device_config, &display->spi), cleanup_bus, TAG, "spi device add failed");
    ESP_GOTO_ON_ERROR(init_panel(display), cleanup_device, TAG, "panel init failed");
    ESP_GOTO_ON_ERROR(st7789_display_set_backlight(display, true), cleanup_device, TAG, "backlight on failed");

    ESP_LOGI(TAG, "Initialized ST7789 display %ux%u",
             board->display_width,
             board->display_height);
    *display_out = display;
    return ESP_OK;

cleanup_device:
    spi_bus_remove_device(display->spi);
cleanup_bus:
    spi_bus_free(board->spi_host);
cleanup:
    free(display);
    return ret;
}

void st7789_display_destroy(st7789_display_t *display)
{
    if (display == NULL) {
        return;
    }

    if (display->spi != NULL) {
        spi_bus_remove_device(display->spi);
        spi_bus_free(display->board->spi_host);
    }
    free(display);
}

esp_err_t st7789_display_set_backlight(st7789_display_t *display, bool enabled)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is required");
    const int active = display->board->tft_backlight_on_level;
    return gpio_set_level(display->board->tft_bl, enabled ? active : !active);
}

esp_err_t st7789_display_push_pixels(st7789_display_t *display,
                                     uint16_t x,
                                     uint16_t y,
                                     uint16_t width,
                                     uint16_t height,
                                     const uint16_t *pixels,
                                     size_t pixel_count)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is required");
    ESP_RETURN_ON_FALSE(pixels != NULL, ESP_ERR_INVALID_ARG, TAG, "pixels are required");
    ESP_RETURN_ON_FALSE(rect_is_valid(display, x, y, width, height), ESP_ERR_INVALID_ARG, TAG, "invalid rectangle");
    ESP_RETURN_ON_FALSE(pixel_count == (size_t)width * height, ESP_ERR_INVALID_SIZE, TAG, "pixel count mismatch");

    ESP_RETURN_ON_ERROR(set_window(display, x, y, width, height), TAG, "set window failed");

    size_t remaining = pixel_count;
    const uint16_t *cursor = pixels;
    while (remaining > 0) {
        const size_t chunk_pixels = remaining > ST7789_MAX_CHUNK_PIXELS ? ST7789_MAX_CHUNK_PIXELS : remaining;
        for (size_t i = 0; i < chunk_pixels; ++i) {
            const uint16_t color = cursor[i];
            display->scratch[i] = (uint16_t)((color << 8) | (color >> 8));
        }
        ESP_RETURN_ON_ERROR(write_data(display, (const uint8_t *)display->scratch, chunk_pixels * sizeof(uint16_t)), TAG, "pixel push failed");
        cursor += chunk_pixels;
        remaining -= chunk_pixels;
    }

    return ESP_OK;
}

esp_err_t st7789_display_draw_rect(st7789_display_t *display,
                                   uint16_t x,
                                   uint16_t y,
                                   uint16_t width,
                                   uint16_t height,
                                   uint16_t color565)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is required");
    ESP_RETURN_ON_FALSE(rect_is_valid(display, x, y, width, height), ESP_ERR_INVALID_ARG, TAG, "invalid rectangle");

    const size_t total_pixels = (size_t)width * height;
    const uint16_t color = (uint16_t)((color565 << 8) | (color565 >> 8));
    for (size_t i = 0; i < ST7789_MAX_CHUNK_PIXELS; ++i) {
        display->scratch[i] = color;
    }

    ESP_RETURN_ON_ERROR(set_window(display, x, y, width, height), TAG, "set window failed");

    size_t remaining = total_pixels;
    while (remaining > 0) {
        const size_t chunk_pixels = remaining > ST7789_MAX_CHUNK_PIXELS ? ST7789_MAX_CHUNK_PIXELS : remaining;
        ESP_RETURN_ON_ERROR(write_data(display, (const uint8_t *)display->scratch, chunk_pixels * sizeof(uint16_t)), TAG, "rect draw failed");
        remaining -= chunk_pixels;
    }

    return ESP_OK;
}

esp_err_t st7789_display_fill(st7789_display_t *display, uint16_t color565)
{
    ESP_RETURN_ON_FALSE(display != NULL, ESP_ERR_INVALID_ARG, TAG, "display is required");
    return st7789_display_draw_rect(display, 0, 0, display->board->display_width, display->board->display_height, color565);
}

esp_err_t st7789_display_draw_pixel(st7789_display_t *display, uint16_t x, uint16_t y, uint16_t color565)
{
    return st7789_display_draw_rect(display, x, y, 1, 1, color565);
}

uint16_t st7789_display_width(const st7789_display_t *display)
{
    return display ? display->board->display_width : 0;
}

uint16_t st7789_display_height(const st7789_display_t *display)
{
    return display ? display->board->display_height : 0;
}
