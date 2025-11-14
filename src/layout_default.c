#include "layout_default.h"

const uint8_t MATRIX_LAYOUT[ROW_COUNT][COL_COUNT] = {
    {KC_4, KC_5, KC_7, KC_6, KC_8, KC_9, KC_0},          // Row "1"
    {KC_R, KC_T, KC_U, KC_Y, KC_I, KC_O, KC_P},          // Row "2"
    {KC_F, KC_G, KC_COMMA, KC_H, KC_DOT, KC_L, KC_ENTER},// Row "3"
    {KC_3, KC_E, KC_C, KC_D, KC_LSHIFT, KC_M, KC_UP},    // Row "4"
    {KC_2, KC_ESCAPE, KC_LALT, KC_TAB, KC_V, KC_LCTRL, KC_BACKSPACE}, // Row "5"
    {KC_1, KC_Q, KC_FN, KC_Z, KC_B, KC_N, KC_RSHIFT},    // Row "6"
};

const uint8_t DISCRETE_LAYOUT[DISCRETE_COUNT] = {
    KC_W,            // FN1
    KC_A,            // FN2
    KC_S,            // FN3
    KC_X,            // FN4
    KC_J,            // FN5
    KC_K,            // FN6
    KC_LEFT_CLICK,   // FN8
    KC_MOUSE_DOWN,   // FN9
    KC_MOUSE_UP,     // FN10
    KC_MOUSE_RIGHT,  // FN11
    KC_MOUSE_LEFT,   // FN12
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

const uint8_t LED_COLOR_DEFAULT[3] = {0, 0, 5};
const uint8_t LED_COLOR_ACTIVE[3] = {5, 0, 0};
const uint8_t LED_COLOR_ERROR[3] = {5, 5, 0};