#include "stream.h"

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "stream";

#define PART_BOUNDARY "frame"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %lld.%06lld\r\n\r\n";

esp_err_t stream_handler(httpd_req_t* req) {
    esp_err_t res;
    char part_buf[128];

    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

    ESP_LOGI(TAG, "Stream started");

    while (true) {
        camera_fb_t* fb = esp_camera_fb_get();
        if (!fb) {
            ESP_LOGE(TAG, "Camera capture failed");
            res = ESP_FAIL;
            break;
        }

        int64_t fr_start = esp_timer_get_time();

        res = httpd_resp_send_chunk(req, STREAM_BOUNDARY, strlen(STREAM_BOUNDARY));
        if (res != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        size_t hlen = snprintf(part_buf, sizeof(part_buf), STREAM_PART, fb->len, fr_start / 1000000,
                               fr_start % 1000000);
        res = httpd_resp_send_chunk(req, part_buf, hlen);
        if (res != ESP_OK) {
            esp_camera_fb_return(fb);
            break;
        }

        res = httpd_resp_send_chunk(req, (const char*)fb->buf, fb->len);
        esp_camera_fb_return(fb);
        if (res != ESP_OK) {
            break;
        }

        int64_t fr_end = esp_timer_get_time();
        ESP_LOGD(TAG, "Frame %uKB %ums", fb->len / 1024, (uint32_t)((fr_end - fr_start) / 1000));
    }

    ESP_LOGI(TAG, "Stream ended");
    return res;
}
