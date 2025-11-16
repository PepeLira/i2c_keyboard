#ifndef POWER_BUTTON_H
#define POWER_BUTTON_H

#include <stdint.h>

typedef enum {
    POWER_STATE_IDLE,           // Waiting for button press
    POWER_STATE_HOLDING,        // Button held, timing the 3s window
    POWER_STATE_DISCHARGED      // Power pin actively holding the line low
} power_state_t;

void power_button_init(void);
void power_button_update(void);
power_state_t power_button_get_state(void);
void gpio_reset_to_default(uint pin);

#endif
