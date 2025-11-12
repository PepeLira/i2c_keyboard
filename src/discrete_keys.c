#include "discrete_keys.h"

#include "pico/stdlib.h"

static inline uint8_t cursor_code_for_index(uint8_t idx) {
    switch (idx) {
    case CURSOR_UP_INDEX: return CURSOR_UP;
    case CURSOR_DOWN_INDEX: return CURSOR_DOWN;
    case CURSOR_LEFT_INDEX: return CURSOR_LEFT;
    case CURSOR_RIGHT_INDEX: return CURSOR_RIGHT;
    case CURSOR_CENTER_INDEX: return CURSOR_CENTER;
    default: return 0;
    }
}

static inline uint8_t cursor_bit_for_index(uint8_t idx) {
    switch (idx) {
    case CURSOR_UP_INDEX: return CURSOR_BIT_UP;
    case CURSOR_DOWN_INDEX: return CURSOR_BIT_DOWN;
    case CURSOR_LEFT_INDEX: return CURSOR_BIT_LEFT;
    case CURSOR_RIGHT_INDEX: return CURSOR_BIT_RIGHT;
    case CURSOR_CENTER_INDEX: return CURSOR_BIT_CENTER;
    default: return 0;
    }
}

void discrete_keys_init(discrete_state_t *state) {
    for (uint8_t i = 0; i < DISCRETE_KEY_COUNT; ++i) {
        gpio_init(DISCRETE_PINS[i]);
        gpio_set_dir(DISCRETE_PINS[i], GPIO_IN);
        gpio_pull_down(DISCRETE_PINS[i]);
        state->pressed[i] = false;
        state->debounce_ticks[i] = 0;
    }
}

void discrete_keys_scan(discrete_state_t *state, fifo_t *fifo, uint8_t *mod_mask,
                        uint8_t *cursor_state, uint8_t timestamp) {
    const uint32_t now = to_ms_since_boot(get_absolute_time());
    for (uint8_t i = 0; i < DISCRETE_KEY_COUNT; ++i) {
        bool pressed = gpio_get(DISCRETE_PINS[i]);
        bool old_pressed = state->pressed[i];
        if (pressed != old_pressed) {
            if ((now - state->debounce_ticks[i]) < DEBOUNCE_MS) {
                continue;
            }
            state->debounce_ticks[i] = now;
            state->pressed[i] = pressed;

            uint8_t keycode = discrete_layout[i];
            if (keycode == KEY_NONE) {
                continue;
            }

            uint8_t cursor_code = cursor_code_for_index(i);
            uint8_t cursor_bit = cursor_bit_for_index(i);

            if (cursor_code) {
                if (pressed) {
                    *cursor_state |= cursor_bit;
                } else {
                    *cursor_state &= (uint8_t)(~cursor_bit);
                }
                fifo_event_t cursor_event = {
                    .type = EVENT_CURSOR,
                    .code = cursor_code,
                    .mods = *mod_mask,
                    .timestamp = timestamp,
                };
                (void)fifo_push(fifo, &cursor_event);
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
}
