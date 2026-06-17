#include "stream.h"

#include <string.h>

#include "esp_camera.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char* TAG = "stream";

#define PART_BOUNDARY "frame"
static const char* STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char* STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char* STREAM_PART =
    "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %lld.%06lld\r\n\r\n";

static volatile float s_fps = 0.0f;
static volatile size_t s_fb_size = 0;
static volatile uint32_t s_frame_count = 0;
static volatile uint32_t s_clients = 0;

void stream_get_stats(float* fps, size_t* fb_size, uint32_t* frame_count, uint32_t* clients) {
    *fps = s_fps;
    *fb_size = s_fb_size;
    *frame_count = s_frame_count;
    *clients = s_clients;
}

esp_err_t stream_handler(httpd_req_t* req) {
    esp_err_t res;
    char part_buf[128];

    res = httpd_resp_set_type(req, STREAM_CONTENT_TYPE);
    if (res != ESP_OK) {
        return res;
    }

    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "X-Framerate", "60");

    s_clients++;
    ESP_LOGI(TAG, "Stream client connected (%lu active)", (unsigned long)s_clients);

    int64_t fps_window_start = esp_timer_get_time();
    uint32_t fps_frame_count = 0;

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
        s_fb_size = fb->len;
        esp_camera_fb_return(fb);

        if (res != ESP_OK) {
            break;
        }

        fps_frame_count++;
        s_frame_count++;

        int64_t now = esp_timer_get_time();
        int64_t elapsed = now - fps_window_start;
        if (elapsed >= 1000000) {
            s_fps = (float)fps_frame_count * 1000000.0f / (float)elapsed;
            fps_frame_count = 0;
            fps_window_start = now;
        }
    }

    s_clients--;
    ESP_LOGI(TAG, "Stream client disconnected (%lu active)", (unsigned long)s_clients);
    return res;
}
