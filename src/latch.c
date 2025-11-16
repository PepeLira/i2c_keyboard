#include "latch.h"

#include "config.h"
#include "event.h"
#include "hardware/gpio.h"

static latch_status_t current_status = LATCH_OPEN;

static void apply_gpio_mode(latch_status_t status) {
    gpio_init(POWER_BUTTON);
    gpio_set_dir(POWER_BUTTON, GPIO_IN);

    switch (status) {
        case LATCH_CLOSED:
            gpio_pull_up(POWER_BUTTON);
            break;
        case LATCH_TRANSITION:
            gpio_pull_up(POWER_BUTTON);
            break;
        case LATCH_OPEN:
        default:
            gpio_disable_pulls(POWER_BUTTON);
            break;
    }
}

void latch_init(void) {
    current_status = LATCH_OPEN;
    apply_gpio_mode(current_status);
}

void latch_set_status(latch_status_t status, uint32_t timestamp_ms) {
    if (status == current_status) {
        return;
    }

    current_status = status;
    apply_gpio_mode(status);

    const event_t ev = {
        .type = EVENT_LATCH_CHANGED,
        .timestamp_ms = timestamp_ms,
        .data.u32 = (uint32_t)status,
    };
    event_publish(&ev);
}

latch_status_t latch_get_status(void) {
    return current_status;
}
