#ifndef POWER_FSM_H
#define POWER_FSM_H

#include "event.h"
#include "latch.h"

typedef enum {
    POWER_MODE_USB_ONLY = 0,
    POWER_MODE_RP_ONLY,
    POWER_MODE_SYSTEM_ON,
    POWER_MODE_SHUTDOWN
} power_mode_t;

void power_fsm_init(void);
void power_fsm_handle_event(const event_t *event);
power_mode_t power_fsm_get_mode(void);

#endif
