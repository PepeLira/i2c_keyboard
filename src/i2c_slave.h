#ifndef I2C_SLAVE_H
#define I2C_SLAVE_H

#include <stdbool.h>
#include <stdint.h>

// I2C slave configuration
#define I2C_SLAVE_SDA_GPIO 0
#define I2C_SLAVE_SCL_GPIO 1
#define I2C_SLAVE_DEFAULT_ADDRESS 0x20
#define I2C_SLAVE_BAUDRATE 100000  // 100 kHz

// Status register bit masks
#define I2C_STATUS_POWER_BUTTON (1 << 0)
#define I2C_STATUS_MODIFIER_BUTTON (1 << 1)

/**
 * Initialize the I2C slave interface.
 * 
 * @param address The I2C slave address (7-bit)
 */
void i2c_slave_init(uint8_t address);

/**
 * Update the button states that will be reported via I2C.
 * This function should be called from the main loop whenever button states change.
 * 
 * @param power_pressed true if power button is pressed, false otherwise
 * @param modifier_pressed true if modifier button is pressed, false otherwise
 */
void i2c_slave_update_button_states(bool power_pressed, bool modifier_pressed);

/**
 * Get the current status byte.
 * This is primarily for testing/debugging purposes.
 * 
 * @return The current status byte
 */
uint8_t i2c_slave_get_status(void);

#endif  // I2C_SLAVE_H
