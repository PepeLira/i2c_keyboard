#include "i2c_slave.h"
#include "hardware/i2c.h"
#include "hardware/irq.h"
#include "pico/stdlib.h"

// Use I2C0 peripheral
#define I2C_INSTANCE i2c0

// Status register - volatile because it's accessed in IRQ context
static volatile uint8_t status_register = 0;

// I2C slave IRQ handler
static void i2c_slave_irq_handler(void) {
    uint32_t status = i2c0->hw->intr_stat;
    
    // Check if master is reading from us (RD_REQ)
    if (status & I2C_IC_INTR_STAT_R_RD_REQ_BITS) {
        // Send the current status byte
        i2c0->hw->data_cmd = status_register;
        
        // Clear the RD_REQ interrupt
        i2c0->hw->clr_rd_req;
    }
    
    // Check if master sent us data (RX_FULL)
    if (status & I2C_IC_INTR_STAT_R_RX_FULL_BITS) {
        // Read and discard data (we don't accept writes in this implementation)
        (void)i2c0->hw->data_cmd;
    }
    
    // Clear STOP_DET interrupt if set
    if (status & I2C_IC_INTR_STAT_R_STOP_DET_BITS) {
        i2c0->hw->clr_stop_det;
    }
}

void i2c_slave_init(uint8_t address) {
    // Initialize I2C pins
    gpio_set_function(I2C_SLAVE_SDA_GPIO, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SLAVE_SCL_GPIO, GPIO_FUNC_I2C);
    
    // Enable pull-ups on I2C pins (required for I2C)
    gpio_pull_up(I2C_SLAVE_SDA_GPIO);
    gpio_pull_up(I2C_SLAVE_SCL_GPIO);
    
    // Initialize I2C peripheral at specified baudrate
    i2c_init(I2C_INSTANCE, I2C_SLAVE_BAUDRATE);
    
    // Disable I2C to configure it
    i2c0->hw->enable = 0;
    
    // Set slave address
    i2c0->hw->sar = address;
    
    // Configure as slave (clear MASTER_MODE and IC_SLAVE_DISABLE)
    i2c0->hw->con = I2C_IC_CON_IC_SLAVE_DISABLE_BITS | I2C_IC_CON_IC_RESTART_EN_BITS |
                    I2C_IC_CON_TX_EMPTY_CTRL_BITS;
    i2c0->hw->con &= ~(I2C_IC_CON_MASTER_MODE_BITS | I2C_IC_CON_IC_SLAVE_DISABLE_BITS);
    
    // Enable interrupts for slave operations
    i2c0->hw->intr_mask = I2C_IC_INTR_MASK_M_RD_REQ_BITS |
                          I2C_IC_INTR_MASK_M_RX_FULL_BITS |
                          I2C_IC_INTR_MASK_M_STOP_DET_BITS;
    
    // Enable I2C
    i2c0->hw->enable = 1;
    
    // Set up the IRQ handler
    irq_set_exclusive_handler(I2C0_IRQ, i2c_slave_irq_handler);
    irq_set_enabled(I2C0_IRQ, true);
    
    // Initialize status register to 0 (no buttons pressed)
    status_register = 0;
}

void i2c_slave_update_button_states(bool power_pressed, bool modifier_pressed) {
    // Build the status byte atomically
    uint8_t new_status = 0;
    
    if (power_pressed) {
        new_status |= I2C_STATUS_POWER_BUTTON;
    }
    
    if (modifier_pressed) {
        new_status |= I2C_STATUS_MODIFIER_BUTTON;
    }
    
    // Update the status register
    // This is atomic on ARM Cortex-M0+ for byte writes
    status_register = new_status;
}

uint8_t i2c_slave_get_status(void) {
    return status_register;
}
