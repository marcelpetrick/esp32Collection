#include "button_input.h"

#include "board_config.h"
#include "driver/gpio.h"
#include "esp_check.h"
#include "esp_log.h"
#include "esp_timer.h"

static const char *TAG = "button_input";

static uint64_t now_ms(void)
{
    return (uint64_t)(esp_timer_get_time() / 1000);
}

static gpio_num_t button_gpio(button_id_t id)
{
    const board_config_t *board = board_config_get();
    switch (id) {
    case BUTTON_ID_LEFT:
        return board->button_left;
    case BUTTON_ID_RIGHT:
        return board->button_right;
    default:
        return GPIO_NUM_NC;
    }
}

static bool gpio_has_internal_pull(gpio_num_t gpio)
{
    return gpio >= GPIO_NUM_0 && gpio <= GPIO_NUM_33;
}

static bool read_pressed(button_id_t id)
{
    return board_button_is_active_level(gpio_get_level(button_gpio(id)));
}

static esp_err_t configure_button(button_id_t id)
{
    const gpio_num_t gpio = button_gpio(id);
    ESP_RETURN_ON_FALSE(gpio != GPIO_NUM_NC, ESP_ERR_INVALID_ARG, TAG, "invalid button gpio");

    gpio_config_t config = {
        .pin_bit_mask = 1ULL << gpio,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = gpio_has_internal_pull(gpio) ? GPIO_PULLUP_ENABLE : GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_RETURN_ON_ERROR(gpio_config(&config), TAG, "button gpio config failed");
    ESP_LOGI(TAG, "%s button on GPIO%d, pullup=%s",
             button_input_name(id),
             gpio,
             gpio_has_internal_pull(gpio) ? "internal" : "external-or-none");
    return ESP_OK;
}

esp_err_t button_input_init(button_input_t *input, uint32_t debounce_ms)
{
    ESP_RETURN_ON_FALSE(input != NULL, ESP_ERR_INVALID_ARG, TAG, "input is required");

    input->debounce_ms = debounce_ms;
    ESP_RETURN_ON_ERROR(configure_button(BUTTON_ID_LEFT), TAG, "left button config failed");
    ESP_RETURN_ON_ERROR(configure_button(BUTTON_ID_RIGHT), TAG, "right button config failed");

    const uint64_t timestamp = now_ms();
    for (button_id_t id = BUTTON_ID_LEFT; id < BUTTON_ID_COUNT; ++id) {
        const bool pressed = read_pressed(id);
        input->buttons[id].stable_pressed = pressed;
        input->buttons[id].last_raw_pressed = pressed;
        input->buttons[id].last_raw_change_ms = timestamp;
    }

    ESP_LOGI(TAG, "Initialized buttons with %lums debounce", (unsigned long)debounce_ms);
    return ESP_OK;
}

esp_err_t button_input_poll(button_input_t *input,
                            button_event_t *events,
                            size_t max_events,
                            size_t *event_count)
{
    ESP_RETURN_ON_FALSE(input != NULL, ESP_ERR_INVALID_ARG, TAG, "input is required");
    ESP_RETURN_ON_FALSE(event_count != NULL, ESP_ERR_INVALID_ARG, TAG, "event_count is required");
    ESP_RETURN_ON_FALSE(events != NULL || max_events == 0, ESP_ERR_INVALID_ARG, TAG, "events are required");

    *event_count = 0;
    const uint64_t timestamp = now_ms();

    for (button_id_t id = BUTTON_ID_LEFT; id < BUTTON_ID_COUNT; ++id) {
        button_state_t *state = &input->buttons[id];
        const bool raw_pressed = read_pressed(id);

        if (raw_pressed != state->last_raw_pressed) {
            state->last_raw_pressed = raw_pressed;
            state->last_raw_change_ms = timestamp;
        }

        if (raw_pressed != state->stable_pressed &&
            timestamp - state->last_raw_change_ms >= input->debounce_ms) {
            state->stable_pressed = raw_pressed;
            if (*event_count < max_events) {
                events[*event_count] = (button_event_t) {
                    .id = id,
                    .type = raw_pressed ? BUTTON_EVENT_PRESSED : BUTTON_EVENT_RELEASED,
                    .timestamp_ms = timestamp,
                };
                ++(*event_count);
            }
        }
    }

    return ESP_OK;
}

const char *button_input_name(button_id_t id)
{
    switch (id) {
    case BUTTON_ID_LEFT:
        return "left";
    case BUTTON_ID_RIGHT:
        return "right";
    default:
        return "unknown";
    }
}

bool button_input_is_pressed(const button_input_t *input, button_id_t id)
{
    if (input == NULL || id >= BUTTON_ID_COUNT) {
        return false;
    }

    return input->buttons[id].stable_pressed;
}
