#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdint.h>
#include "fifo.h"

typedef struct {
    fifo_t *fifo;
    uint8_t *mod_mask;
    uint8_t *cursor_state;
    uint8_t *scan_rate_hz;
} i2c_context_t;

typedef void (*led_write_callback_t)(uint8_t r, uint8_t g, uint8_t b);

void i2c_keyboard_init(i2c_context_t *ctx, led_write_callback_t led_cb);
void i2c_keyboard_set_error(bool error_flag);

#endif // I2C_SLAVE_H
