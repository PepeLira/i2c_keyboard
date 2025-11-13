#ifndef MATRIX_SCAN_H
#define MATRIX_SCAN_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"

typedef void (*matrix_event_cb)(uint16_t key_index, bool pressed, void *ctx);

typedef struct {
    uint8_t row_pins[ROW_COUNT];
    uint8_t col_pins[COL_COUNT];
    uint16_t debounce_us;
} matrix_config_t;

typedef struct {
    matrix_config_t config;
    uint32_t stable_state[ROW_COUNT];
    uint32_t debounce_state[ROW_COUNT];
    uint64_t last_change_us[ROW_COUNT * COL_COUNT];
} matrix_state_t;

void matrix_state_init(matrix_state_t *state, const matrix_config_t *config);
void matrix_scan_poll(matrix_state_t *state, uint64_t now_us, matrix_event_cb cb, void *ctx);

#endif // MATRIX_SCAN_H
