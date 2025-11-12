#include "pico/stdlib.h"

#include "config.h"
#include "discrete_keys.h"
#include "fifo.h"
#include "i2c_slave.h"
#include "matrix_scan.h"
#include "neopixel.h"
#include "protocol.h"

int main() {
    stdio_init_all();

    fifo_t event_fifo;
    fifo_init(&event_fifo);

    matrix_state_t matrix_state;
    matrix_init(&matrix_state);

    discrete_state_t discrete_state;
    discrete_keys_init(&discrete_state);

    neopixel_init();

    uint8_t mod_mask = 0;
    uint8_t cursor_state = 0;
    uint8_t scan_rate = SCAN_RATE_HZ;
    uint8_t timestamp = 0;

    i2c_context_t ctx = {
        .fifo = &event_fifo,
        .mod_mask = &mod_mask,
        .cursor_state = &cursor_state,
        .scan_rate_hz = &scan_rate,
    };

    i2c_keyboard_init(&ctx, neopixel_set_color);

    absolute_time_t last_scan = get_absolute_time();

    while (true) {
        tight_loop_contents();
        absolute_time_t now = get_absolute_time();
        uint32_t interval_us = 0;
        if (scan_rate == 0) {
            scan_rate = 1;
        }
        interval_us = 1000000u / scan_rate;
        if (absolute_time_diff_us(last_scan, now) >= (int64_t)interval_us) {
            timestamp++;
            matrix_scan(&matrix_state, &event_fifo, &mod_mask, timestamp);
            discrete_keys_scan(&discrete_state, &event_fifo, &mod_mask, &cursor_state, timestamp);
            neopixel_update_modifiers(mod_mask);
            last_scan = now;
        }
    }
}
