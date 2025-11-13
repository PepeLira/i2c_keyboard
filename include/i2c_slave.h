#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdbool.h>
#include <stdint.h>
#include "protocol.h"

typedef struct {
    uint8_t status;
    uint8_t mod_mask;
    uint16_t scan_rate_hz;
    uint8_t cfg_flags;
    uint8_t cursor_state;
    uint8_t led_state[3];
    uint16_t timestamp_high;
} register_bank_t;

typedef struct i2c_slave_context i2c_slave_context_t;

typedef bool (*i2c_fifo_pop_cb)(keyboard_event_t *event, void *user_data);
typedef size_t (*i2c_fifo_count_cb)(void *user_data);
typedef bool (*i2c_fifo_empty_cb)(void *user_data);

i2c_slave_context_t *i2c_slave_init(register_bank_t *regs,
                                    i2c_fifo_pop_cb pop_cb,
                                    i2c_fifo_count_cb count_cb,
                                    void *user_data);
void i2c_slave_task(i2c_slave_context_t *ctx);
void i2c_slave_notify_event(i2c_slave_context_t *ctx);

#endif // I2C_SLAVE_H
