#include "power_fsm.h"

#include <stdio.h>

#include "event.h"
#include "latch.h"

static power_mode_t current_mode = POWER_MODE_RP_ONLY;

static void publish_power_mode(power_mode_t mode, uint32_t timestamp_ms) {
    const event_t ev = {
        .type = EVENT_POWER_MODE_CHANGED,
        .timestamp_ms = timestamp_ms,
        .data.u32 = (uint32_t)mode,
    };
    event_publish(&ev);
}

void power_fsm_init(void) {
    current_mode = POWER_MODE_RP_ONLY;
    latch_init();
    publish_power_mode(current_mode, 0);
}

power_mode_t power_fsm_get_mode(void) {
    return current_mode;
}

void power_fsm_handle_event(const event_t *event) {
    if (!event) {
        return;
    }

    switch (event->type) {
        case EVENT_BUTTON_PRESSED:
            latch_set_status(LATCH_TRANSITION, event->timestamp_ms);
            break;
        case EVENT_BUTTON_HOLD_1S:
            latch_set_status(LATCH_CLOSED, event->timestamp_ms);
            if (current_mode != POWER_MODE_SYSTEM_ON) {
                current_mode = POWER_MODE_SYSTEM_ON;
                publish_power_mode(current_mode, event->timestamp_ms);
                printf("[PowerFSM] System on after 1s hold\n");
            }
            break;
        case EVENT_BUTTON_HOLD_3S:
            latch_set_status(LATCH_OPEN, event->timestamp_ms);
            if (current_mode != POWER_MODE_SHUTDOWN) {
                current_mode = POWER_MODE_SHUTDOWN;
                publish_power_mode(current_mode, event->timestamp_ms);
                printf("[PowerFSM] Shutdown after 3s hold\n");
            }
            break;
        case EVENT_BUTTON_RELEASED:
            if (current_mode == POWER_MODE_SHUTDOWN) {
                current_mode = POWER_MODE_RP_ONLY;
                publish_power_mode(current_mode, event->timestamp_ms);
            }
            latch_set_status(LATCH_OPEN, event->timestamp_ms);
            break;
        default:
            break;
    }
}
