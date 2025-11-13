#include "matrix_scan.h"

#include <stddef.h>
#include <string.h>

#include "hardware/gpio.h"
#include "pico/stdlib.h"

#define INDEX_OF(row, col) ((row) * COL_COUNT + (col))

static void init_row_pin(uint8_t gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_OUT);
    gpio_put(gpio, 0);
}

static void init_col_pin(uint8_t gpio) {
    gpio_init(gpio);
    gpio_set_dir(gpio, GPIO_IN);
    gpio_pull_down(gpio);
}

void matrix_state_init(matrix_state_t *state, const matrix_config_t *config) {
    memset(state, 0, sizeof(*state));
    state->config.debounce_us = config->debounce_us;
    for (size_t i = 0; i < ROW_COUNT; ++i) {
        state->config.row_pins[i] = config->row_pins[i];
        init_row_pin(state->config.row_pins[i]);
    }
    for (size_t i = 0; i < COL_COUNT; ++i) {
        state->config.col_pins[i] = config->col_pins[i];
        init_col_pin(state->config.col_pins[i]);
    }
    memset(state->stable_state, 0, sizeof(state->stable_state));
    memset(state->debounce_state, 0, sizeof(state->debounce_state));
    memset(state->last_change_us, 0, sizeof(state->last_change_us));
}

void matrix_scan_poll(matrix_state_t *state, uint64_t now_us, matrix_event_cb cb, void *ctx) {
    for (size_t row = 0; row < ROW_COUNT; ++row) {
        uint8_t row_pin = state->config.row_pins[row];
        // drive current row high
        gpio_put(row_pin, 1);
        sleep_us(2);

        uint32_t raw_row = 0;
        for (size_t col = 0; col < COL_COUNT; ++col) {
            uint8_t col_pin = state->config.col_pins[col];
            bool pressed = gpio_get(col_pin);
            if (pressed) {
                raw_row |= (1u << col);
            }
        }

        uint32_t prev_raw_row = state->debounce_state[row];
        uint32_t stable_row = state->stable_state[row];

        uint32_t changed = raw_row ^ prev_raw_row;
        if (changed) {
            for (size_t col = 0; col < COL_COUNT; ++col) {
                if (changed & (1u << col)) {
                    size_t index = INDEX_OF(row, col);
                    state->last_change_us[index] = now_us;
                }
            }
            state->debounce_state[row] = raw_row;
        }

        uint32_t pending = raw_row ^ stable_row;
        if (pending) {
            for (size_t col = 0; col < COL_COUNT; ++col) {
                if (pending & (1u << col)) {
                    size_t index = INDEX_OF(row, col);
                    if ((now_us - state->last_change_us[index]) >= state->config.debounce_us) {
                        bool pressed = (raw_row & (1u << col)) != 0;
                        if (pressed) {
                            stable_row |= (1u << col);
                        } else {
                            stable_row &= ~(1u << col);
                        }
                        state->stable_state[row] = stable_row;
                        if (cb) {
                            cb((uint16_t)index, pressed, ctx);
                        }
                    }
                }
            }
        }

        gpio_put(row_pin, 0);
    }
}
