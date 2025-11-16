#include "power_button.h"
#include "config.h"
#include "neopixel.h"
#include <stdio.h>
#include <stdbool.h>
#include "pico/stdlib.h"
#include "hardware/gpio.h"

// State machine variables
static power_state_t current_state = POWER_STATE_IDLE;
static uint32_t button_press_time = 0;
static uint32_t next_release_sample_ms = 0;

void power_button_init(void) {
    // Power button is already initialized as pull-up input in main.c
    current_state = POWER_STATE_IDLE;
}

power_state_t power_button_get_state(void) {
    return current_state;
}

void gpio_reset_to_default(uint pin) {
    gpio_deinit(pin);
    gpio_set_function(pin, GPIO_FUNC_NULL);
    gpio_disable_pulls(pin);
    gpio_set_dir(pin, GPIO_IN);
}

void power_button_update(void) {
    uint32_t current_time = time_us_32() / 1000;  // Convert to milliseconds
    
    if (current_state == POWER_STATE_DISCHARGED) {
        if (button_press_time != 0) {
            gpio_set_dir(POWER_BUTTON, GPIO_OUT);
            gpio_put(POWER_BUTTON, 0);
            neopixel_off();
            button_press_time = 0;  // Use as a flag that discharge has started
            next_release_sample_ms = current_time + DISCHARGE_SAMPLE_MS;
            printf("[Power] Power pin driven low after 3s hold\n");
            return;
        }

        if ((int32_t)(current_time - next_release_sample_ms) >= 0) {
            // Release the line briefly to detect a button release.
            gpio_set_dir(POWER_BUTTON, GPIO_IN);
            gpio_pull_up(POWER_BUTTON);
            busy_wait_us_32(5);
            uint32_t released = gpio_get(POWER_BUTTON);

            if (released == 1) {
                current_state = POWER_STATE_IDLE;
                button_press_time = 0;
                neopixel_set_color(DEFAULT_COLOR);
                printf("[Power] Button released, returning to idle\n");
            } else {
                gpio_set_dir(POWER_BUTTON, GPIO_OUT);
                gpio_put(POWER_BUTTON, 0);
                next_release_sample_ms = current_time + DISCHARGE_SAMPLE_MS;
            }
        }
        return;
    }

    uint32_t button_state = gpio_get(POWER_BUTTON);

    switch (current_state) {
        case POWER_STATE_IDLE:
            if (button_state == 0) {
                button_press_time = current_time;
                current_state = POWER_STATE_HOLDING;
                printf("[Power] Button press detected\n");
            }
            break;

        case POWER_STATE_HOLDING:
            if (button_state == 1) {
                current_state = POWER_STATE_IDLE;
                button_press_time = 0;
                neopixel_set_color(default_COLOR);
                printf("[Power] Button released before shutdown\n");
                break;
            }

            uint32_t held_ms = current_time - button_press_time;
            uint32_t phase = held_ms % BLINK_PERIOD_MS;
            if (phase < BLINK_ON_MS) {
                neopixel_set_color(SHUTDOWN_COLOR);  // Red pulse for half the period
            } else {
                neopixel_set_color(DEFAULT_COLOR);  // Otherwise turn green 
            }

            if (held_ms >= POWER_HOLD_MS) {
                current_state = POWER_STATE_DISCHARGED;
                printf("[Power] Long press met, discharging line\n");
            }
            break;

        default:
            break;
    }
}
