#include "button.h"
#include "config.h"
#include "led_controller.h"
#include "pico/stdlib.h"
#include "power_latch.h"
#include "switch_tracker.h"
#include "tick.h"

#define DEBOUNCE_MS 30
#define STARTUP_WINDOW_MS 1000
#define FIRST_PRESS_HOLD_MS 1000
#define LONG_PRESS_MS 3000

static void process_switch_event(switch_event_t event, uint32_t now_ms) {
    switch (event) {
        case SWITCH_EVENT_FIRST_PRESS:
            power_latch_close();
            break;
        case SWITCH_EVENT_LONG_PRESS:
            power_latch_open();
            break;
        case SWITCH_EVENT_SHORT_PRESS:
            led_controller_pulse_short_press(now_ms);
            break;
        case SWITCH_EVENT_NONE:
        default:
            break;
    }
}

int main() {
    stdio_init_all();

    power_latch_init(CONFIG_POWER_LATCH_GPIO);

    button_t power_button = {0};
    button_init(&power_button, CONFIG_POWER_LATCH_GPIO, false, DEBOUNCE_MS, true, false);

    button_t modifier_button = {0};
    button_init(&modifier_button, CONFIG_MODIFIER_GPIO, true, DEBOUNCE_MS, false, true);

    led_controller_init(CONFIG_LED_GPIO);
    tick_service_init(1000);

    switch_tracker_t tracker;
    switch_tracker_init(&tracker, STARTUP_WINDOW_MS, FIRST_PRESS_HOLD_MS, LONG_PRESS_MS);

    bool latch_opened_after_startup = false;

    while (true) {
        if (tick_consume()) {
            uint32_t now_ms = tick_now_ms();

            button_update(&power_button, now_ms);
            button_update(&modifier_button, now_ms);

            bool power_pressed = button_is_pressed(&power_button);
            bool modifier_pressed = button_is_pressed(&modifier_button);

            led_controller_set_power_pressed(power_pressed);
            led_controller_set_modifier(modifier_pressed);

            switch_event_t event = switch_tracker_tick(&tracker, power_pressed, now_ms);
            process_switch_event(event, now_ms);

            if (!latch_opened_after_startup && switch_tracker_startup_elapsed(&tracker) &&
                !switch_tracker_should_hold_latch(&tracker) &&
                !switch_tracker_first_press_pending(&tracker) && !power_pressed) {
                power_latch_open();
                latch_opened_after_startup = true;
            }

            led_controller_tick(now_ms);
        }

        tight_loop_contents();
    }

    return 0;
}
