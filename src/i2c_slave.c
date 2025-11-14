#include "i2c_slave.h"

#include <string.h>

#include "config.h"
#include "fifo.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "hardware/regs/i2c.h"
#include "neopixel.h"
#include "pico/stdlib.h"

#define I2C_PORT i2c0

struct i2c_slave_context {
    register_bank_t *regs;
    i2c_fifo_pop_cb pop_cb;
    i2c_fifo_count_cb count_cb;
    void *user_data;
    uint8_t pointer;
    uint8_t sub_index;
    bool pointer_valid;
    keyboard_event_t current_event;
    uint8_t current_event_offset;
    bool current_event_valid;
};

static struct i2c_slave_context g_ctx;
static bool ctx_initialized = false;

static void update_fifo_status(struct i2c_slave_context *ctx) {
    size_t count = ctx->count_cb ? ctx->count_cb(ctx->user_data) : 0;
    ctx->regs->status &= ~(STATUS_FIFO_EMPTY | STATUS_FIFO_FULL);
    if (count == 0) {
        ctx->regs->status |= STATUS_FIFO_EMPTY;
    } else if (count >= FIFO_SIZE) {
        ctx->regs->status |= STATUS_FIFO_FULL;
    }
}

static void set_int_line(bool active) {
#if USE_INT_PIN
    gpio_put(INT_GPIO, active ? 0 : 1);
#else
    (void)active;
#endif
}

static void handle_led_write(struct i2c_slave_context *ctx) {
    neopixel_set_rgb(ctx->regs->led_state[0], ctx->regs->led_state[1], ctx->regs->led_state[2]);
}

static void load_next_event(struct i2c_slave_context *ctx) {
    if (!ctx->current_event_valid) {
        if (ctx->pop_cb && ctx->pop_cb(&ctx->current_event, ctx->user_data)) {
            ctx->current_event_valid = true;
            ctx->current_event_offset = 0;
            update_fifo_status(ctx);
            if (ctx->count_cb && ctx->count_cb(ctx->user_data) == 0) {
                set_int_line(false);
            }
        } else {
            memset(&ctx->current_event, 0, sizeof(ctx->current_event));
            ctx->current_event.type = EVENT_NOP;
            ctx->current_event_valid = true;
            ctx->current_event_offset = 0;
            update_fifo_status(ctx);
            set_int_line(false);
        }
    }
}

static uint8_t read_register_byte(struct i2c_slave_context *ctx) {
    uint8_t value = 0x00;
    switch (ctx->pointer) {
        case REG_DEV_ID:
            value = DEV_ID;
            ctx->pointer++;
            break;
        case REG_FW_VERSION:
            if (ctx->sub_index == 0) {
                value = FW_VERSION_MAJOR;
                ctx->sub_index = 1;
            } else {
                value = FW_VERSION_MINOR;
                ctx->sub_index = 0;
                ctx->pointer++;
            }
            break;
        case REG_STATUS:
            update_fifo_status(ctx);
            value = ctx->regs->status;
            ctx->pointer++;
            break;
        case REG_MOD_MASK:
            value = ctx->regs->mod_mask;
            ctx->pointer++;
            break;
        case REG_FIFO_COUNT: {
            size_t count = ctx->count_cb ? ctx->count_cb(ctx->user_data) : 0;
            value = (uint8_t)count;
            ctx->pointer++;
            break;
        }
        case REG_FIFO_POP:
            load_next_event(ctx);
            if (ctx->current_event_offset >= sizeof(keyboard_event_t)) {
                ctx->current_event_valid = false;
                ctx->current_event_offset = 0;
                load_next_event(ctx);
            }
            value = ((uint8_t *)&ctx->current_event)[ctx->current_event_offset++];
            if (ctx->current_event_offset >= sizeof(keyboard_event_t)) {
                ctx->current_event_valid = false;
                ctx->current_event_offset = 0;
            }
            break;
        case REG_CFG_FLAGS:
            value = ctx->regs->cfg_flags;
            ctx->pointer++;
            break;
        case REG_CURSOR_STATE:
            value = ctx->regs->cursor_state;
            ctx->pointer++;
            break;
        case REG_LED_STATE:
            value = ctx->regs->led_state[ctx->sub_index];
            ctx->sub_index++;
            if (ctx->sub_index >= 3) {
                ctx->sub_index = 0;
                ctx->pointer++;
            }
            break;
        case REG_SCAN_RATE:
            value = (uint8_t)(ctx->regs->scan_rate_hz & 0xFF);
            ctx->pointer++;
            break;
        case REG_TS_HIGH:
            value = (uint8_t)((ctx->regs->timestamp_high >> (ctx->sub_index * 8)) & 0xFF);
            ctx->sub_index++;
            if (ctx->sub_index >= 2) {
                ctx->sub_index = 0;
                ctx->pointer++;
            }
            break;
        default:
            value = 0x00;
            ctx->pointer++;
            break;
    }
    return value;
}

static void write_register_byte(struct i2c_slave_context *ctx, uint8_t value) {
    switch (ctx->pointer) {
        case REG_CFG_FLAGS:
            ctx->regs->cfg_flags = value;
            ctx->pointer++;
            break;
        case REG_MOD_MASK:
            ctx->regs->mod_mask = value;
            ctx->pointer++;
            break;
        case REG_LED_STATE:
            ctx->regs->led_state[ctx->sub_index] = value;
            ctx->sub_index++;
            if (ctx->sub_index >= 3) {
                ctx->sub_index = 0;
                handle_led_write(ctx);
                ctx->pointer++;
            }
            break;
        case REG_SCAN_RATE:
            ctx->regs->scan_rate_hz = value;
            ctx->pointer++;
            break;
        default:
            ctx->pointer++;
            break;
    }
}

static void i2c_slave_irq_handler(void) {
    struct i2c_slave_context *ctx = &g_ctx;
    i2c_hw_t *hw = I2C_PORT->hw;

    while (hw->intr_stat) {
        uint32_t status = hw->intr_stat;
        if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
            uint8_t data = (uint8_t)(hw->data_cmd & I2C_IC_DATA_CMD_DAT_BITS);
            if (!ctx->pointer_valid) {
                ctx->pointer = data;
                ctx->sub_index = 0;
                ctx->pointer_valid = true;
            } else {
                write_register_byte(ctx, data);
            }
        }
        if (status & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
            uint8_t value = read_register_byte(ctx);
            hw->data_cmd = value;
            hw->clr_rd_req;
        }
        if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
            hw->clr_stop_det;
            ctx->pointer_valid = false;
            ctx->sub_index = 0;
            ctx->current_event_valid = false;
        }
        if (status & I2C_IC_INTR_STAT_R_START_DET_BITS) {
            hw->clr_start_det;
        }
        if (status & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
            hw->clr_tx_abrt;
        }
    }
}

i2c_slave_context_t *i2c_slave_init(register_bank_t *regs,
                                    i2c_fifo_pop_cb pop_cb,
                                    i2c_fifo_count_cb count_cb,
                                    void *user_data) {
    if (ctx_initialized) {
        return &g_ctx;
    }
    memset(&g_ctx, 0, sizeof(g_ctx));
    g_ctx.regs = regs;
    g_ctx.pop_cb = pop_cb;
    g_ctx.count_cb = count_cb;
    g_ctx.user_data = user_data;
    g_ctx.current_event_valid = false;

    gpio_init(SDA_GPIO);
    gpio_init(SCL_GPIO);
    gpio_set_function(SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_GPIO);
    gpio_pull_up(SCL_GPIO);

#if USE_INT_PIN
    gpio_init(INT_GPIO);
    gpio_set_dir(INT_GPIO, GPIO_OUT);
    gpio_put(INT_GPIO, 1);
#endif

    i2c_init(I2C_PORT, 100 * 1000);
    i2c_set_slave_mode(I2C_PORT, true, I2C_ADDR_BASE);

    irq_set_exclusive_handler(I2C0_IRQ, i2c_slave_irq_handler);
    irq_set_enabled(I2C0_IRQ, true);

    ctx_initialized = true;
    return &g_ctx;
}

void i2c_slave_task(i2c_slave_context_t *ctx) {
    (void)ctx;
}

void i2c_slave_notify_event(i2c_slave_context_t *ctx) {
    if (!ctx) {
        return;
    }
    ctx->regs->status &= ~STATUS_FIFO_EMPTY;
    update_fifo_status(ctx);
#if USE_INT_PIN
    set_int_line(true);
#endif
}
