#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "driver/gpio.h"
#include "driver/spi_master.h"

#define BOARD_NAME "TENSTAR T-Display ESP32"
#define BOARD_DISPLAY_DRIVER "ST7789"
#define BOARD_DISPLAY_WIDTH 135
#define BOARD_DISPLAY_HEIGHT 240
#define BOARD_DISPLAY_X_OFFSET 52
#define BOARD_DISPLAY_Y_OFFSET 40

#define BOARD_TFT_SPI_HOST SPI2_HOST
#define BOARD_TFT_SPI_CLOCK_HZ (20 * 1000 * 1000)
#define BOARD_TFT_PIN_MOSI GPIO_NUM_19
#define BOARD_TFT_PIN_SCLK GPIO_NUM_18
#define BOARD_TFT_PIN_CS GPIO_NUM_5
#define BOARD_TFT_PIN_DC GPIO_NUM_16
#define BOARD_TFT_PIN_RST GPIO_NUM_23
#define BOARD_TFT_PIN_BL GPIO_NUM_4
#define BOARD_TFT_BACKLIGHT_ON_LEVEL 1

#define BOARD_BUTTON_LEFT_GPIO GPIO_NUM_0
#define BOARD_BUTTON_RIGHT_GPIO GPIO_NUM_35
#define BOARD_BUTTON_ACTIVE_LEVEL 0

#define BOARD_BATTERY_ADC_GPIO GPIO_NUM_34
#define BOARD_BATTERY_DIVIDER_RATIO 2.0f

#define BOARD_FLASH_SIZE_BYTES (16u * 1024u * 1024u)
#define BOARD_SERIAL_BAUD 115200

typedef struct {
    const char *name;
    const char *display_driver;
    uint16_t display_width;
    uint16_t display_height;
    uint16_t display_x_offset;
    uint16_t display_y_offset;
    spi_host_device_t spi_host;
    uint32_t spi_clock_hz;
    gpio_num_t tft_mosi;
    gpio_num_t tft_sclk;
    gpio_num_t tft_cs;
    gpio_num_t tft_dc;
    gpio_num_t tft_rst;
    gpio_num_t tft_bl;
    int tft_backlight_on_level;
    gpio_num_t button_left;
    gpio_num_t button_right;
    int button_active_level;
    gpio_num_t battery_adc;
    float battery_divider_ratio;
    uint32_t expected_flash_size;
    uint32_t serial_baud;
} board_config_t;

const board_config_t *board_config_get(void);
bool board_button_is_active_level(int level);
void board_config_log(void);
