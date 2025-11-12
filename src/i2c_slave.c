#include "i2c_slave.h"

#include "config.h"
#include "protocol.h"
#include "hardware/gpio.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"

static i2c_context_t *ctx_ref = NULL;
static led_write_callback_t led_callback = NULL;
static volatile bool error_flag = false;

static uint8_t current_reg = 0;
static bool reg_addr_valid = false;
static fifo_event_t fifo_cached_event;
static uint8_t fifo_cached_index = 0;
static bool fifo_cached_valid = false;
static uint8_t led_state[3] = {0};
static uint8_t cfg_flags = 0;

static uint8_t read_register_value(void);
static void write_register_value(uint8_t value);
static uint8_t read_fifo_pop(void);
static void handle_rd_request(void);
static void handle_rx_full(void);
static void handle_stop_det(void);
static void handle_tx_abrt(void);
static void i2c_irq(void);

void i2c_keyboard_init(i2c_context_t *ctx, led_write_callback_t led_cb) {
    ctx_ref = ctx;
    led_callback = led_cb;

    gpio_set_function(SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(SCL_GPIO, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_GPIO);
    gpio_pull_up(SCL_GPIO);

    i2c_init(i2c0, 100 * 1000);
    i2c_set_slave_mode(i2c0, true, I2C_ADDR_BASE);

    irq_set_exclusive_handler(I2C0_IRQ, i2c_irq);
    irq_set_enabled(I2C0_IRQ, true);

    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RD_REQ_BITS |
                          I2C_IC_INTR_MASK_M_RX_FULL_BITS |
                          I2C_IC_INTR_MASK_M_STOP_DET_BITS |
                          I2C_IC_INTR_MASK_M_TX_ABRT_BITS;
}

void i2c_keyboard_set_error(bool flag) {
    error_flag = flag;
}

static void handle_rd_request(void) {
    uint8_t value = read_register_value();
    i2c0->hw->data_cmd = value;
    (void)i2c0->hw->clr_rd_req;
}

static void handle_rx_full(void) {
    uint8_t value = (uint8_t)(i2c0->hw->data_cmd & I2C_IC_DATA_CMD_DAT_MASK);
    if (!reg_addr_valid) {
        current_reg = value;
        reg_addr_valid = true;
        fifo_cached_valid = false;
        fifo_cached_index = 0;
    } else {
        write_register_value(value);
        current_reg++;
    }
}

static void handle_stop_det(void) {
    reg_addr_valid = false;
    (void)i2c0->hw->clr_stop_det;
}

static void handle_tx_abrt(void) {
    (void)i2c0->hw->clr_tx_abrt;
}

static void i2c_irq(void) {
    uint32_t status = i2c0->hw->intr_stat;
    if (status & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
        handle_rd_request();
    }
    if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        handle_rx_full();
    }
    if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
        handle_stop_det();
    }
    if (status & I2C_IC_INTR_STAT_R_TX_ABRT_BITS) {
        handle_tx_abrt();
    }
}

static uint8_t status_register(void) {
    uint8_t status = 0;
    if (!ctx_ref) {
        return STATUS_ERROR;
    }
    if (fifo_is_empty(ctx_ref->fifo)) {
        status |= STATUS_FIFO_EMPTY;
    }
    if (fifo_is_full(ctx_ref->fifo)) {
        status |= STATUS_FIFO_FULL;
    }
    if (ctx_ref->mod_mask) {
        status |= STATUS_MOD_VALID;
    }
    if (error_flag) {
        status |= STATUS_ERROR;
    }
    return status;
}

static uint8_t read_register_value(void) {
    uint8_t value = 0;
    switch (current_reg) {
    case REG_DEV_ID:
        value = I2C_DEV_ID;
        break;
    case REG_FW_VERSION_MAJOR:
        value = FW_VERSION_MAJOR;
        break;
    case REG_FW_VERSION_MINOR:
        value = FW_VERSION_MINOR;
        break;
    case REG_STATUS:
        value = status_register();
        break;
    case REG_MOD_MASK:
        value = ctx_ref && ctx_ref->mod_mask ? *ctx_ref->mod_mask : 0;
        break;
    case REG_FIFO_COUNT:
        value = ctx_ref && ctx_ref->fifo ? ctx_ref->fifo->count : 0;
        break;
    case REG_FIFO_POP:
        value = read_fifo_pop();
        return value;
    case REG_CFG_FLAGS:
        value = cfg_flags;
        break;
    case REG_CURSOR:
        value = ctx_ref && ctx_ref->cursor_state ? *ctx_ref->cursor_state : 0;
        break;
    case REG_LED_STATE:
        value = led_state[0];
        break;
    case REG_LED_STATE + 1:
        value = led_state[1];
        break;
    case REG_LED_STATE + 2:
        value = led_state[2];
        break;
    case REG_SCAN_RATE:
        value = ctx_ref && ctx_ref->scan_rate_hz ? *ctx_ref->scan_rate_hz : 0;
        break;
    default:
        value = 0;
        break;
    }
    current_reg++;
    return value;
}

static uint8_t read_fifo_pop(void) {
    if (!ctx_ref || !ctx_ref->fifo) {
        return 0;
    }
    if (!fifo_cached_valid) {
        if (!fifo_pop(ctx_ref->fifo, &fifo_cached_event)) {
            fifo_cached_event.type = EVENT_NOP;
            fifo_cached_event.code = 0;
            fifo_cached_event.mods = ctx_ref->mod_mask ? *ctx_ref->mod_mask : 0;
            fifo_cached_event.timestamp = 0;
        }
        fifo_cached_index = 0;
        fifo_cached_valid = true;
    }
    uint8_t *raw = (uint8_t *)&fifo_cached_event;
    uint8_t value = raw[fifo_cached_index++];
    if (fifo_cached_index >= sizeof(fifo_event_t)) {
        fifo_cached_valid = false;
    }
    return value;
}

static void write_register_value(uint8_t value) {
    switch (current_reg) {
    case REG_CFG_FLAGS:
        cfg_flags = value;
        break;
    case REG_LED_STATE:
    case REG_LED_STATE + 1:
    case REG_LED_STATE + 2:
        led_state[current_reg - REG_LED_STATE] = value;
        if ((current_reg - REG_LED_STATE) == 2 && led_callback) {
            // Expect GRB order from host
            led_callback(led_state[1], led_state[0], led_state[2]);
        }
        break;
    case REG_SCAN_RATE:
        if (ctx_ref && ctx_ref->scan_rate_hz) {
            *ctx_ref->scan_rate_hz = value;
        }
        break;
    default:
        break;
    }
}
