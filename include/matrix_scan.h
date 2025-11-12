#ifndef MATRIX_SCAN_H
#define MATRIX_SCAN_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "fifo.h"
#include "layout_default.h"

typedef struct {
    bool pressed[MATRIX_ROWS][MATRIX_COLS];
    uint32_t debounce_ticks[MATRIX_ROWS][MATRIX_COLS];
} matrix_state_t;

void matrix_init(matrix_state_t *state);
void matrix_scan(matrix_state_t *state, fifo_t *fifo, uint8_t *mod_mask, uint8_t timestamp);

#endif // MATRIX_SCAN_H
