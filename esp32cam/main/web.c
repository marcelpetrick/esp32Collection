#include "web.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "esp_camera.h"
#include "esp_log.h"
#include "stream.h"

static const char* TAG = "web";

extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_end");

esp_err_t index_handler(httpd_req_t* req) {
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "identity");
    size_t len = index_html_end - index_html_start;
    return httpd_resp_send(req, (const char*)index_html_start, len);
}

esp_err_t stats_handler(httpd_req_t* req) {
    float fps;
    size_t fb_size;
    uint32_t frame_count, clients;
    stream_get_stats(&fps, &fb_size, &frame_count, &clients);

    char json[128];
    snprintf(json, sizeof(json), "{\"fps\":%.1f,\"fb_size\":%u,\"frames\":%u,\"clients\":%u}", fps,
             (unsigned)fb_size, (unsigned)frame_count, (unsigned)clients);

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, json);
}

esp_err_t ctrl_handler(httpd_req_t* req) {
    char query[256];
    char var[64], val_str[16];
    int val;

    if (httpd_req_get_url_query_str(req, query, sizeof(query)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }

    if (httpd_query_key_value(query, "var", var, sizeof(var)) != ESP_OK ||
        httpd_query_key_value(query, "val", val_str, sizeof(val_str)) != ESP_OK) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, NULL);
        return ESP_FAIL;
    }

    val = atoi(val_str);

    sensor_t* s = esp_camera_sensor_get();
    if (!s) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_FAIL;
    }

    int res = 0;

    if (!strcmp(var, "framesize")) {
        res = s->set_framesize(s, (framesize_t)val);
    } else if (!strcmp(var, "quality")) {
        res = s->set_quality(s, val);
    } else if (!strcmp(var, "brightness")) {
        res = s->set_brightness(s, val);
    } else if (!strcmp(var, "contrast")) {
        res = s->set_contrast(s, val);
    } else if (!strcmp(var, "saturation")) {
        res = s->set_saturation(s, val);
    } else if (!strcmp(var, "sharpness")) {
        res = s->set_sharpness(s, val);
    } else if (!strcmp(var, "denoise")) {
        res = s->set_denoise(s, val);
    } else if (!strcmp(var, "gainceiling")) {
        res = s->set_gainceiling(s, (gainceiling_t)val);
    } else if (!strcmp(var, "colorbar")) {
        res = s->set_colorbar(s, val);
    } else if (!strcmp(var, "whitebal")) {
        res = s->set_whitebal(s, val);
    } else if (!strcmp(var, "gain_ctrl")) {
        res = s->set_gain_ctrl(s, val);
    } else if (!strcmp(var, "exposure_ctrl")) {
        res = s->set_exposure_ctrl(s, val);
    } else if (!strcmp(var, "hmirror")) {
        res = s->set_hmirror(s, val);
    } else if (!strcmp(var, "vflip")) {
        res = s->set_vflip(s, val);
    } else if (!strcmp(var, "aec2")) {
        res = s->set_aec2(s, val);
    } else if (!strcmp(var, "awb_gain")) {
        res = s->set_awb_gain(s, val);
    } else if (!strcmp(var, "agc_gain")) {
        res = s->set_agc_gain(s, val);
    } else if (!strcmp(var, "aec_value")) {
        res = s->set_aec_value(s, val);
    } else if (!strcmp(var, "special_effect")) {
        res = s->set_special_effect(s, val);
    } else if (!strcmp(var, "wb_mode")) {
        res = s->set_wb_mode(s, val);
    } else if (!strcmp(var, "ae_level")) {
        res = s->set_ae_level(s, val);
    } else if (!strcmp(var, "dcw")) {
        res = s->set_dcw(s, val);
    } else if (!strcmp(var, "bpc")) {
        res = s->set_bpc(s, val);
    } else if (!strcmp(var, "wpc")) {
        res = s->set_wpc(s, val);
    } else if (!strcmp(var, "raw_gma")) {
        res = s->set_raw_gma(s, val);
    } else if (!strcmp(var, "lenc")) {
        res = s->set_lenc(s, val);
    } else {
        ESP_LOGW(TAG, "Unknown control: %s", var);
        httpd_resp_send_err(req, HTTPD_404_NOT_FOUND, NULL);
        return ESP_FAIL;
    }

    if (res != 0) {
        ESP_LOGW(TAG, "Control %s=%d failed (%d)", var, val, res);
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, NULL);
        return ESP_FAIL;
    }

    ESP_LOGD(TAG, "ctrl %s=%d", var, val);
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    return httpd_resp_sendstr(req, "OK");
}
