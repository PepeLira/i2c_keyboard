#include "button.h"

#include <stdbool.h>

#include "config.h"
#include "event.h"
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define BUTTON_HOLD_SHORT_MS 1000

static bool debounced_pressed = false;
static uint32_t last_change_ms = 0;
static uint32_t press_start_ms = 0;
static bool hold_1s_emitted = false;
static bool hold_3s_emitted = false;

void button_init(void) {
    gpio_init(POWER_BUTTON);
    gpio_set_dir(POWER_BUTTON, GPIO_IN);
    gpio_pull_up(POWER_BUTTON);

    debounced_pressed = false;
    last_change_ms = 0;
    press_start_ms = 0;
    hold_1s_emitted = false;
    hold_3s_emitted = false;
}

void button_on_tick_1ms(uint32_t timestamp_ms) {
    bool raw_pressed = gpio_get(POWER_BUTTON) == 0;

    if (raw_pressed != debounced_pressed) {
        if ((timestamp_ms - last_change_ms) >= DEBOUNCE_MS) {
            debounced_pressed = raw_pressed;
            last_change_ms = timestamp_ms;

            if (debounced_pressed) {
                press_start_ms = timestamp_ms;
                hold_1s_emitted = false;
                hold_3s_emitted = false;
                const event_t ev = {
                    .type = EVENT_BUTTON_PRESSED,
                    .timestamp_ms = timestamp_ms,
                };
                event_publish(&ev);
            } else {
                const event_t ev = {
                    .type = EVENT_BUTTON_RELEASED,
                    .timestamp_ms = timestamp_ms,
                };
                event_publish(&ev);
            }
        }
    } else {
        last_change_ms = timestamp_ms;
    }

    if (debounced_pressed) {
        const uint32_t held_ms = timestamp_ms - press_start_ms;

        if (!hold_1s_emitted && held_ms >= BUTTON_HOLD_SHORT_MS) {
            hold_1s_emitted = true;
            const event_t ev = {
                .type = EVENT_BUTTON_HOLD_1S,
                .timestamp_ms = timestamp_ms,
            };
            event_publish(&ev);
        }

        if (!hold_3s_emitted && held_ms >= POWER_HOLD_MS) {
            hold_3s_emitted = true;
            const event_t ev = {
                .type = EVENT_BUTTON_HOLD_3S,
                .timestamp_ms = timestamp_ms,
            };
            event_publish(&ev);
        }
    }
}
