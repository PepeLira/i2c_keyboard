#include "matrix_scan.h"

#include "pico/stdlib.h"

static inline bool read_column(uint8_t gpio) {
    return gpio_get(gpio);
}

void matrix_init(matrix_state_t *state) {
    for (uint8_t r = 0; r < MATRIX_ROWS; ++r) {
        gpio_init(ROW_PINS[r]);
        gpio_set_dir(ROW_PINS[r], GPIO_OUT);
        gpio_put(ROW_PINS[r], 0);
    }
    for (uint8_t c = 0; c < MATRIX_COLS; ++c) {
        gpio_init(COL_PINS[c]);
        gpio_set_dir(COL_PINS[c], GPIO_IN);
        gpio_pull_down(COL_PINS[c]);
    }

    for (uint8_t r = 0; r < MATRIX_ROWS; ++r) {
        for (uint8_t c = 0; c < MATRIX_COLS; ++c) {
            state->pressed[r][c] = false;
            state->debounce_ticks[r][c] = 0;
        }
    }
}

void matrix_scan(matrix_state_t *state, fifo_t *fifo, uint8_t *mod_mask, uint8_t timestamp) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    for (uint8_t r = 0; r < MATRIX_ROWS; ++r) {
        gpio_put(ROW_PINS[r], 1);
        sleep_us(5);

        for (uint8_t c = 0; c < MATRIX_COLS; ++c) {
            bool pressed = read_column(COL_PINS[c]);
            bool old_pressed = state->pressed[r][c];

            if (pressed != old_pressed) {
                if ((now - state->debounce_ticks[r][c]) < DEBOUNCE_MS) {
                    continue;
                }
                state->debounce_ticks[r][c] = now;
                state->pressed[r][c] = pressed;

                uint8_t keycode = matrix_layout[r][c];
                if (keycode == KEY_NONE) {
                    continue;
                }

                uint16_t mod_bits = modifier_mask_for_key(keycode);
                fifo_event_t event = {
                    .type = pressed ? EVENT_KEY_DOWN : EVENT_KEY_UP,
                    .code = keycode,
                    .mods = *mod_mask,
                    .timestamp = timestamp,
                };

                if (mod_bits) {
                    if (pressed) {
                        *mod_mask |= (uint8_t)mod_bits;
                    } else {
                        *mod_mask &= (uint8_t)(~mod_bits);
                    }
                    event.type = EVENT_MOD_CHANGE;
                    event.code = (uint8_t)mod_bits;
                    event.mods = *mod_mask;
                }

                (void)fifo_push(fifo, &event);
            }
        }

        gpio_put(ROW_PINS[r], 0);
    }
}
