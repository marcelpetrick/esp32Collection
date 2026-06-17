#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_mac.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "esp_http_server.h"

#include "camera.h"
#include "stream.h"

static const char *TAG = "dillycam";

#define WIFI_SSID     "dillyCam"
#define WIFI_PASS     "foobar"
#define WIFI_CHANNEL  6
#define WIFI_MAX_STA  4

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t *e = data;
        ESP_LOGI(TAG, "Station "MACSTR" joined, AID=%d",
                 MAC2STR(e->mac), e->aid);
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t *e = data;
        ESP_LOGI(TAG, "Station "MACSTR" left, AID=%d",
                 MAC2STR(e->mac), e->aid);
    }
}

static esp_err_t wifi_ap_init(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL));

    wifi_config_t ap_config = {
        .ap = {
            .ssid           = WIFI_SSID,
            .ssid_len       = strlen(WIFI_SSID),
            .channel        = WIFI_CHANNEL,
            .password       = WIFI_PASS,
            .max_connection = WIFI_MAX_STA,
            .authmode       = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg        = { .required = false },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "AP started — SSID:%s pass:%s channel:%d",
             WIFI_SSID, WIFI_PASS, WIFI_CHANNEL);
    return ESP_OK;
}

static esp_err_t http_server_init(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
    /* Stream handler runs long; give it its own stack */
    config.stack_size = 8192;

    ESP_ERROR_CHECK(httpd_start(&server, &config));

    httpd_uri_t stream_uri = {
        .uri      = "/stream",
        .method   = HTTP_GET,
        .handler  = stream_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &stream_uri);

    /* Root redirect to stream */
    httpd_uri_t root_uri = {
        .uri      = "/",
        .method   = HTTP_GET,
        .handler  = stream_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(server, &root_uri);

    ESP_LOGI(TAG, "HTTP server up — http://192.168.4.1/stream");
    return ESP_OK;
}

void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(camera_init());
    ESP_ERROR_CHECK(wifi_ap_init());
    ESP_ERROR_CHECK(http_server_init());

    ESP_LOGI(TAG, "dillyCam ready — connect to '%s' then open http://192.168.4.1/stream",
             WIFI_SSID);
}
