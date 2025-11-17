#include "button.h"
#include "config.h"
#include "led_controller.h"
#include "pico/stdlib.h"
#include "power_latch.h"
#include "switch_tracker.h"
#include "tick.h"

static void process_switch_event(switch_event_t event, uint32_t now_ms) {
    switch (event) {
        case SWITCH_EVENT_FIRST_PRESS:
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

    // Start with a closed latch
    power_latch_close();

    button_t power_button = {0};
    button_init(&power_button, CONFIG_POWER_LATCH_GPIO, false, DEBOUNCE_MS, true, false);

    button_t modifier_button = {0};
    button_init(&modifier_button, CONFIG_MODIFIER_GPIO, false, DEBOUNCE_MS, true, false);

    led_controller_init(CONFIG_LED_GPIO);
    tick_service_init(1000);

    switch_tracker_t tracker;
    switch_tracker_init(&tracker, STARTUP_WINDOW_MS, FIRST_PRESS_HOLD_MS, LONG_PRESS_MS);

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

            led_controller_tick(now_ms);
        }

        tight_loop_contents();
    }

    return 0;
}
