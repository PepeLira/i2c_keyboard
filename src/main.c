#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include "pico/stdlib.h"
#include "pico/time.h"

#include "config.h"
#include "discrete_keys.h"
#include "fifo.h"
#include "i2c_slave.h"
#include "layout_default.h"
#include "matrix_scan.h"
#include "neopixel.h"
#include "protocol.h"

static event_fifo_t g_fifo;
static register_bank_t g_registers;
static matrix_state_t g_matrix_state;
static discrete_key_bank_t g_discrete_bank;
static i2c_slave_context_t *g_i2c_ctx = NULL;
static uint8_t g_modifier_mask = 0;
static uint8_t g_cursor_state = 0;
static uint8_t g_led_current[3] = {0};

static uint8_t mod_bit_for_keycode(uint8_t keycode) {
    switch (keycode) {
        case KC_LCTRL:
            return MOD_LEFT_CTRL;
        case KC_LSHIFT:
            return MOD_LEFT_SHIFT;
        case KC_LALT:
            return MOD_LEFT_ALT;
        case KC_LGUI:
            return MOD_LEFT_GUI;
        case KC_RCTRL:
            return MOD_RIGHT_CTRL;
        case KC_RSHIFT:
            return MOD_RIGHT_SHIFT;
        case KC_RALT:
            return MOD_RIGHT_ALT;
        case KC_RGUI:
            return MOD_RIGHT_GUI;
        default:
            return 0;
    }
}

static void update_led(const uint8_t color[3]) {
    if (memcmp(g_led_current, color, 3) != 0) {
        neopixel_set_rgb(color[0], color[1], color[2]);
        memcpy(g_led_current, color, 3);
        memcpy(g_registers.led_state, color, 3);
    }
}

static void refresh_led_for_state(void) {
    if (g_registers.status & STATUS_ERROR) {
        update_led(LED_COLOR_ERROR);
    } else if (g_modifier_mask) {
        update_led(LED_COLOR_ACTIVE);
    } else {
        update_led(LED_COLOR_DEFAULT);
    }
}

static void push_event(uint8_t type, uint8_t code, uint8_t mod_mask, uint64_t timestamp) {
    keyboard_event_t event = {
        .type = type,
        .code = code,
        .mod_mask = mod_mask,
        .ts_low = (uint8_t)(timestamp & 0xFF),
    };
    if (!fifo_push(&g_fifo, &event)) {
        g_registers.status |= STATUS_ERROR | STATUS_FIFO_FULL;
    } else {
        if (!fifo_is_empty(&g_fifo)) {
            g_registers.status &= ~STATUS_FIFO_EMPTY;
        }
        if (fifo_is_full(&g_fifo)) {
            g_registers.status |= STATUS_FIFO_FULL;
        } else {
            g_registers.status &= ~STATUS_FIFO_FULL;
        }
        if (g_i2c_ctx) {
            g_registers.timestamp_high = (uint16_t)((timestamp >> 8) & 0xFFFF);
            i2c_slave_notify_event(g_i2c_ctx);
        }
    }
}

static void emit_mod_change(uint64_t timestamp) {
    push_event(EVENT_MOD_CHANGE, g_modifier_mask, g_modifier_mask, timestamp);
    g_registers.mod_mask = g_modifier_mask;
    g_registers.status |= STATUS_MOD_VALID;
    refresh_led_for_state();
}

static void handle_key_event(uint8_t keycode, bool pressed, uint64_t timestamp) {
    if (keycode == KC_NONE) {
        return;
    }
    uint8_t mod_bit = mod_bit_for_keycode(keycode);
    bool is_modifier = mod_bit != 0;
    if (is_modifier) {
        uint8_t previous_mask = g_modifier_mask;
        if (pressed) {
            g_modifier_mask |= mod_bit;
        } else {
            g_modifier_mask &= (uint8_t)~mod_bit;
        }
        if (previous_mask != g_modifier_mask) {
            emit_mod_change(timestamp);
        }
    }
    push_event(pressed ? EVENT_KEY_DOWN : EVENT_KEY_UP, keycode, g_modifier_mask, timestamp);
}

static uint8_t cursor_code_from_index(uint16_t index) {
    switch (index) {
        case 0:
            return CURSOR_UP;
        case 1:
            return CURSOR_DOWN;
        case 2:
            return CURSOR_LEFT;
        case 3:
            return CURSOR_RIGHT;
        case 4:
            return CURSOR_CENTER;
        default:
            return 0;
    }
}

static void emit_cursor_event(uint16_t index, bool pressed, uint64_t timestamp) {
    uint8_t cursor_code = cursor_code_from_index(index);
    if (!cursor_code) {
        return;
    }
    if (pressed) {
        g_cursor_state |= (1u << index);
    } else {
        g_cursor_state &= (uint8_t)~(1u << index);
        cursor_code |= CURSOR_RELEASE_FLAG;
    }
    g_registers.cursor_state = g_cursor_state;
    push_event(EVENT_CURSOR, cursor_code, g_modifier_mask, timestamp);
}

static void matrix_event_callback(uint16_t key_index, bool pressed, void *ctx) {
    (void)ctx;
    uint64_t now = time_us_64();
    uint16_t row = key_index / COL_COUNT;
    uint16_t col = key_index % COL_COUNT;
    uint8_t keycode = MATRIX_LAYOUT[row][col];
    handle_key_event(keycode, pressed, now);
}

static void discrete_event_callback(uint16_t key_index, bool pressed, void *ctx) {
    (void)ctx;
    uint64_t now = time_us_64();
    uint8_t keycode = DISCRETE_LAYOUT[key_index];
    if (key_index < 5) {
        handle_key_event(keycode, pressed, now);
        emit_cursor_event(key_index, pressed, now);
    } else {
        handle_key_event(keycode, pressed, now);
    }
}

static bool fifo_pop_cb(keyboard_event_t *event, void *user_data) {
    event_fifo_t *fifo = (event_fifo_t *)user_data;
    bool result = fifo_pop(fifo, event);
    if (result) {
        g_registers.status &= ~STATUS_FIFO_FULL;
        if (fifo_is_empty(fifo)) {
            g_registers.status |= STATUS_FIFO_EMPTY;
        } else {
            g_registers.status &= ~STATUS_FIFO_EMPTY;
        }
    }
    return result;
}

static size_t fifo_count_cb(void *user_data) {
    event_fifo_t *fifo = (event_fifo_t *)user_data;
    return fifo_count(fifo);
}

int main() {
    stdio_init_all();

    fifo_init(&g_fifo);
    matrix_config_t matrix_config = {
        .row_pins = {0},
        .col_pins = {0},
        .debounce_us = DEBOUNCE_US,
    };
    memcpy(matrix_config.row_pins, ROW_PINS, sizeof(matrix_config.row_pins));
    memcpy(matrix_config.col_pins, COL_PINS, sizeof(matrix_config.col_pins));

    matrix_state_init(&g_matrix_state, &matrix_config);
    discrete_keys_init(&g_discrete_bank, DEBOUNCE_US);

    memset(&g_registers, 0, sizeof(g_registers));
    g_registers.status = STATUS_FIFO_EMPTY;
    g_registers.scan_rate_hz = SCAN_RATE_HZ;
    memcpy(g_registers.led_state, LED_COLOR_DEFAULT, 3);

    neopixel_init();
    update_led(LED_COLOR_DEFAULT);

    g_i2c_ctx = i2c_slave_init(&g_registers, fifo_pop_cb, fifo_count_cb, &g_fifo);

    const uint32_t scan_interval_us = 1000000 / SCAN_RATE_HZ;
    absolute_time_t next_scan = make_timeout_time_us(scan_interval_us);

    while (true) {
        uint64_t now = time_us_64();
        matrix_scan_poll(&g_matrix_state, now, matrix_event_callback, NULL);
        discrete_keys_poll(&g_discrete_bank, now, discrete_event_callback, NULL);
        refresh_led_for_state();
        sleep_until(next_scan);
        next_scan = delayed_by_us(next_scan, scan_interval_us);
        tight_loop_contents();
    }
}
