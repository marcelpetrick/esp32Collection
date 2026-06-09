#include "board_config.h"

#include "esp_log.h"

static const char *TAG = "board_config";

static const board_config_t BOARD_CONFIG = {
    .name = BOARD_NAME,
    .display_driver = BOARD_DISPLAY_DRIVER,
    .display_width = BOARD_DISPLAY_WIDTH,
    .display_height = BOARD_DISPLAY_HEIGHT,
    .spi_host = BOARD_TFT_SPI_HOST,
    .spi_clock_hz = BOARD_TFT_SPI_CLOCK_HZ,
    .tft_mosi = BOARD_TFT_PIN_MOSI,
    .tft_sclk = BOARD_TFT_PIN_SCLK,
    .tft_cs = BOARD_TFT_PIN_CS,
    .tft_dc = BOARD_TFT_PIN_DC,
    .tft_rst = BOARD_TFT_PIN_RST,
    .tft_bl = BOARD_TFT_PIN_BL,
    .tft_backlight_on_level = BOARD_TFT_BACKLIGHT_ON_LEVEL,
    .button_left = BOARD_BUTTON_LEFT_GPIO,
    .button_right = BOARD_BUTTON_RIGHT_GPIO,
    .button_active_level = BOARD_BUTTON_ACTIVE_LEVEL,
    .battery_adc = BOARD_BATTERY_ADC_GPIO,
    .battery_divider_ratio = BOARD_BATTERY_DIVIDER_RATIO,
    .expected_flash_size = BOARD_FLASH_SIZE_BYTES,
    .serial_baud = BOARD_SERIAL_BAUD,
};

const board_config_t *board_config_get(void)
{
    return &BOARD_CONFIG;
}

bool board_button_is_active_level(int level)
{
    return level == BOARD_CONFIG.button_active_level;
}

void board_config_log(void)
{
    ESP_LOGI(TAG, "Board: %s", BOARD_CONFIG.name);
    ESP_LOGI(TAG, "Display: %s %ux%u",
             BOARD_CONFIG.display_driver,
             BOARD_CONFIG.display_width,
             BOARD_CONFIG.display_height);
    ESP_LOGI(TAG, "TFT SPI: host=%d clock=%luHz MOSI=%d SCLK=%d CS=%d DC=%d RST=%d BL=%d",
             BOARD_CONFIG.spi_host,
             (unsigned long)BOARD_CONFIG.spi_clock_hz,
             BOARD_CONFIG.tft_mosi,
             BOARD_CONFIG.tft_sclk,
             BOARD_CONFIG.tft_cs,
             BOARD_CONFIG.tft_dc,
             BOARD_CONFIG.tft_rst,
             BOARD_CONFIG.tft_bl);
    ESP_LOGI(TAG, "Buttons: left GPIO%d, right GPIO%d, active level=%d",
             BOARD_CONFIG.button_left,
             BOARD_CONFIG.button_right,
             BOARD_CONFIG.button_active_level);
    ESP_LOGI(TAG, "Battery ADC: GPIO%d, divider ratio=%.2f",
             BOARD_CONFIG.battery_adc,
             BOARD_CONFIG.battery_divider_ratio);
    ESP_LOGI(TAG, "Expected flash: %lu bytes, serial baud: %lu",
             (unsigned long)BOARD_CONFIG.expected_flash_size,
             (unsigned long)BOARD_CONFIG.serial_baud);
}
