#ifndef LAYOUT_DEFAULT_H
#define LAYOUT_DEFAULT_H

#include "config.h"
#include "keycodes.h"
#include "protocol.h"

#define DEFAULT_LAYER_COUNT 1

extern const uint8_t MATRIX_LAYOUT[ROW_COUNT][COL_COUNT];
extern const uint8_t DISCRETE_LAYOUT[DISCRETE_COUNT];
extern const uint8_t MODIFIER_KEYS[];
extern const uint8_t CURSOR_KEY_INDEX[5];
extern const uint8_t LED_COLOR_DEFAULT[3];
extern const uint8_t LED_COLOR_ACTIVE[3];
extern const uint8_t LED_COLOR_ERROR[3];

#endif // LAYOUT_DEFAULT_H
