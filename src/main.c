#include <stdio.h>

#include "button.h"
#include "config.h"
#include "event.h"
#include "neopixel.h"
#include "pico/stdlib.h"
#include "power_fsm.h"

// Binary info for UF2 file
bi_decl(bi_program_description("RP2040 I2C Keyboard with Power Button and NeoPixel"));
bi_decl(bi_program_version_string("1.0.0"));

int main(void) {
    stdio_init_all();
    sleep_ms(100);

    printf("RP2040 Keyboard Firmware Starting...\n");
    printf("Power Button GPIO: %d\n", POWER_BUTTON);
    printf("NeoPixel GPIO: %d\n", NEOPIXEL_GPIO);

    event_init();
    neopixel_init();

    event_subscribe(power_fsm_handle_event);
    event_subscribe(neopixel_handle_event);

    power_fsm_init();
    button_init();

    event_process_pending();

    printf("Firmware ready. Waiting for power button events...\n");

    while (1) {
        const uint32_t now_ms = to_ms_since_boot(get_absolute_time());

        const event_t tick = {
            .type = EVENT_TICK_1MS,
            .timestamp_ms = now_ms,
        };
        event_publish(&tick);

        button_on_tick_1ms(now_ms);
        event_process_pending();

        sleep_ms(1);
    }

    return 0;
}
