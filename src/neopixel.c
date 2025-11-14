#include "neopixel.h"

#include "config.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"

#include "pio/neopixel.pio.h"

static PIO neopixel_pio = pio0;
static int neopixel_sm = -1;
static uint neopixel_offset = 0;

void neopixel_init(void) {
    neopixel_offset = pio_add_program(neopixel_pio, &ws2812_program);
    neopixel_sm = pio_claim_unused_sm(neopixel_pio, true);

    ws2812_program_init(neopixel_pio, neopixel_sm, neopixel_offset, NEOPIXEL_GPIO, 800000.0f, false);
    
    // Small delay for WS2812 reset (needs 50+ us low)
    sleep_us(100);
}

void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (neopixel_sm < 0) {
        return;
    }
    // WS2812 expects GRB format
    uint32_t value = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    pio_sm_put_blocking(neopixel_pio, neopixel_sm, value);
    // Small delay for latch (WS2812 needs 50+ us low after data)
    sleep_us(60);
}
