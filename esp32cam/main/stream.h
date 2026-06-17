#pragma once

#include "esp_http_server.h"

esp_err_t stream_handler(httpd_req_t* req);
void stream_get_stats(float* fps, size_t* fb_size, uint32_t* frame_count, uint32_t* clients);
