#include "neopixel.h"

#include <stdbool.h>

#include "config.h"
#include "event.h"
#include "hardware/clocks.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "neopixel.pio.h"
#include "pico/stdlib.h"

static PIO pio_instance = pio0;
static uint state_machine = 0;

static bool button_active = false;
static bool blink_on = false;
static uint32_t last_blink_ms = 0;
static power_mode_t cached_mode = POWER_MODE_RP_ONLY;

void neopixel_init(void) {
    uint offset = pio_add_program(pio_instance, &ws2812_program);
    state_machine = pio_claim_unused_sm(pio_instance, true);
    ws2812_program_init(pio_instance, state_machine, offset, NEOPIXEL_GPIO, 800000.0f, false);

    cached_mode = POWER_MODE_RP_ONLY;
    button_active = false;
    blink_on = false;
    last_blink_ms = 0;
}

void neopixel_set_color(rgb_color_t color) {
    uint32_t color_f = ((uint32_t)color.g << 16) |
                      ((uint32_t)color.r << 8) |
                      (uint32_t)color.b;
    pio_sm_put_blocking(pio_instance, state_machine, color_f << 8);
}

void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    neopixel_set_color((rgb_color_t){r, g, b});
}

void neopixel_off(void) {
    neopixel_set_color((rgb_color_t){0, 0, 0});
}

static void apply_power_color(void) {
    switch (cached_mode) {
        case POWER_MODE_SYSTEM_ON:
        case POWER_MODE_RP_ONLY:
        case POWER_MODE_USB_ONLY:
            neopixel_set_color(DEFAULT_COLOR);
            break;
        case POWER_MODE_SHUTDOWN:
        default:
            neopixel_off();
            break;
    }
}

static void update_blink(uint32_t timestamp_ms) {
    if (!button_active) {
        return;
    }

    uint32_t phase = (timestamp_ms - last_blink_ms) % BLINK_PERIOD_MS;
    bool should_on = phase < BLINK_ON_MS;
    if (should_on != blink_on) {
        blink_on = should_on;
        if (blink_on) {
            neopixel_set_color(SHUTDOWN_COLOR);
        } else {
            neopixel_off();
        }
    }
}

void neopixel_handle_event(const event_t *event) {
    if (!event) {
        return;
    }

    switch (event->type) {
        case EVENT_POWER_MODE_CHANGED:
            cached_mode = (power_mode_t)event->data.u32;
            apply_power_color();
            break;
        case EVENT_BUTTON_PRESSED:
            button_active = true;
            blink_on = false;
            last_blink_ms = event->timestamp_ms;
            neopixel_set_color(SHUTDOWN_COLOR);
            break;
        case EVENT_BUTTON_RELEASED:
            button_active = false;
            blink_on = false;
            apply_power_color();
            break;
        case EVENT_BUTTON_HOLD_3S:
            button_active = false;
            blink_on = false;
            neopixel_off();
            break;
        case EVENT_TICK_1MS:
            update_blink(event->timestamp_ms);
            break;
        default:
            break;
    }
}
