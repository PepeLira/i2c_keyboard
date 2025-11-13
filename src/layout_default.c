#include "layout_default.h"

const uint8_t MATRIX_LAYOUT[ROW_COUNT][COL_COUNT] = {
    {KC_ESCAPE, KC_1, KC_2, KC_3, KC_4, KC_5},
    {KC_TAB, KC_Q, KC_W, KC_E, KC_R, KC_T},
    {KC_CAPSLOCK, KC_A, KC_S, KC_D, KC_F, KC_G},
    {KC_LSHIFT, KC_Z, KC_X, KC_C, KC_V, KC_B},
    {KC_LCTRL, KC_LGUI, KC_LALT, KC_SPACE, KC_SPACE, KC_SPACE},
    {KC_F1, KC_F2, KC_F3, KC_F4, KC_F5, KC_F6},
    {KC_GRAVE, KC_6, KC_7, KC_8, KC_9, KC_0},
};

const uint8_t DISCRETE_LAYOUT[DISCRETE_COUNT] = {
    KC_UP,
    KC_DOWN,
    KC_LEFT,
    KC_RIGHT,
    KC_ENTER,
    KC_RCTRL,
    KC_RSHIFT,
    KC_RALT,
    KC_RGUI,
    KC_MACRO0,
    KC_MACRO1,
};

const uint8_t MODIFIER_KEYS[] = {
    KC_LCTRL,
    KC_LSHIFT,
    KC_LALT,
    KC_LGUI,
    KC_RCTRL,
    KC_RSHIFT,
    KC_RALT,
    KC_RGUI,
    KC_NONE,
};

const uint8_t CURSOR_KEY_INDEX[5] = {0, 1, 2, 3, 4};

const uint8_t LED_COLOR_DEFAULT[3] = {0x00, 0x08, 0x00};
const uint8_t LED_COLOR_ACTIVE[3] = {0x08, 0x00, 0x00};
const uint8_t LED_COLOR_ERROR[3] = {0x08, 0x08, 0x00};
