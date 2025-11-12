#ifndef DISCRETE_KEYS_H
#define DISCRETE_KEYS_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "fifo.h"
#include "layout_default.h"

typedef struct {
    bool pressed[DISCRETE_KEY_COUNT];
    uint32_t debounce_ticks[DISCRETE_KEY_COUNT];
} discrete_state_t;

void discrete_keys_init(discrete_state_t *state);
void discrete_keys_scan(discrete_state_t *state, fifo_t *fifo, uint8_t *mod_mask,
                        uint8_t *cursor_state, uint8_t timestamp);

#endif // DISCRETE_KEYS_H
