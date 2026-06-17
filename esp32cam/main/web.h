#pragma once

#include "esp_http_server.h"

esp_err_t index_handler(httpd_req_t* req);
esp_err_t ctrl_handler(httpd_req_t* req);
esp_err_t stats_handler(httpd_req_t* req);
