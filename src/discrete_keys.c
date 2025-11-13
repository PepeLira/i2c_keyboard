#include "discrete_keys.h"

#include <stddef.h>

#include "hardware/gpio.h"

void discrete_keys_init(discrete_key_bank_t *bank, uint16_t debounce_us) {
    bank->debounce_us = debounce_us;
    for (size_t i = 0; i < DISCRETE_COUNT; ++i) {
        bank->keys[i].gpio = DISCRETE_PINS[i];
        bank->keys[i].stable_state = false;
        bank->keys[i].raw_state = false;
        bank->keys[i].last_transition_us = 0;
        gpio_init(bank->keys[i].gpio);
        gpio_set_dir(bank->keys[i].gpio, GPIO_IN);
        gpio_pull_down(bank->keys[i].gpio);
    }
}

void discrete_keys_poll(discrete_key_bank_t *bank, uint64_t now_us, discrete_event_cb cb, void *ctx) {
    for (size_t i = 0; i < DISCRETE_COUNT; ++i) {
        discrete_key_t *key = &bank->keys[i];
        bool pressed = gpio_get(key->gpio);
        if (pressed != key->raw_state) {
            key->raw_state = pressed;
            key->last_transition_us = now_us;
        }
        if ((now_us - key->last_transition_us) >= bank->debounce_us) {
            if (key->stable_state != key->raw_state) {
                key->stable_state = key->raw_state;
                if (cb) {
                    cb((uint16_t)i, key->stable_state, ctx);
                }
            }
        }
    }
}
