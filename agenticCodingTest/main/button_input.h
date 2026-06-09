#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "esp_err.h"

typedef enum {
    BUTTON_ID_LEFT = 0,
    BUTTON_ID_RIGHT,
    BUTTON_ID_COUNT,
} button_id_t;

typedef enum {
    BUTTON_EVENT_PRESSED = 0,
    BUTTON_EVENT_RELEASED,
} button_event_type_t;

typedef struct {
    button_id_t id;
    button_event_type_t type;
    uint64_t timestamp_ms;
} button_event_t;

typedef struct {
    bool stable_pressed;
    bool last_raw_pressed;
    uint64_t last_raw_change_ms;
} button_state_t;

typedef struct {
    button_state_t buttons[BUTTON_ID_COUNT];
    uint32_t debounce_ms;
} button_input_t;

esp_err_t button_input_init(button_input_t *input, uint32_t debounce_ms);
esp_err_t button_input_poll(button_input_t *input,
                            button_event_t *events,
                            size_t max_events,
                            size_t *event_count);
const char *button_input_name(button_id_t id);
bool button_input_is_pressed(const button_input_t *input, button_id_t id);
