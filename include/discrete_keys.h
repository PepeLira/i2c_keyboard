#ifndef DISCRETE_KEYS_H
#define DISCRETE_KEYS_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef void (*discrete_event_cb)(uint16_t key_index, bool pressed, void *ctx);

typedef struct {
    uint8_t gpio;
    bool stable_state;
    bool raw_state;
    uint64_t last_transition_us;
} discrete_key_t;

typedef struct {
    discrete_key_t keys[DISCRETE_COUNT];
    uint16_t debounce_us;
} discrete_key_bank_t;

void discrete_keys_init(discrete_key_bank_t *bank, uint16_t debounce_us);
void discrete_keys_poll(discrete_key_bank_t *bank, uint64_t now_us, discrete_event_cb cb, void *ctx);

#endif // DISCRETE_KEYS_H
