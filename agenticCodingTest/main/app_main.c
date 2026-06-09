#include "board_config.h"
#include "st7789_display.h"

#include "esp_chip_info.h"
#include "esp_err.h"
#include "esp_flash.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "tdisplay_games";

void app_main(void)
{
    esp_chip_info_t chip_info;
    uint32_t flash_size = 0;
    int chip_rev_major = 0;
    int chip_rev_minor = 0;
    st7789_display_t *display = NULL;

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

    uint32_t tick = 0;
    while (true) {
        ESP_LOGI(TAG, "heartbeat %lu", (unsigned long)tick++);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}
