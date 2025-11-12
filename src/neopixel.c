#include "neopixel.h"

#include "config.h"
#include "protocol.h"
#include "hardware/clocks.h"
#include "hardware/pio.h"
#include "pico/stdlib.h"
#include "ws2812.pio.h"

static PIO neopixel_pio = pio0;
static uint neopixel_sm = 0;
static uint neopixel_offset = 0;

static void ws2812_program_custom_init(PIO pio, uint sm, uint offset, uint pin, float freq, bool rgbw) {
    pio_sm_config c = ws2812_program_get_default_config(offset);
    sm_config_set_sideset_pins(&c, pin);
    sm_config_set_out_shift(&c, false, true, rgbw ? 32 : 24);
    sm_config_set_fifo_join(&c, PIO_FIFO_JOIN_TX);

    const float div = clock_get_hz(clk_sys) / (freq * (ws2812_T1 + ws2812_T2 + ws2812_T3));
    sm_config_set_clkdiv(&c, div);

    pio_gpio_init(pio, pin);
    pio_sm_set_consecutive_pindirs(pio, sm, pin, 1, true);
    pio_sm_init(pio, sm, offset, &c);
    pio_sm_set_enabled(pio, sm, true);
}

static inline uint32_t pack_color(uint8_t r, uint8_t g, uint8_t b) {
    return ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
}

void neopixel_init(void) {
    neopixel_offset = pio_add_program(neopixel_pio, &ws2812_program);
    neopixel_sm = pio_claim_unused_sm(neopixel_pio, true);
    ws2812_program_custom_init(neopixel_pio, neopixel_sm, neopixel_offset, NEOPIXEL_GPIO, 800000.0f, false);
    neopixel_set_color(0, 0, 0);
}

void neopixel_set_color(uint8_t r, uint8_t g, uint8_t b) {
    uint32_t color = pack_color(r, g, b);
    pio_sm_put_blocking(neopixel_pio, neopixel_sm, color << 8u);
}

void neopixel_update_modifiers(uint8_t mod_mask) {
    uint8_t r = 0, g = 0, b = 0;
    if (mod_mask & MOD_CTRL) {
        r = 64;
    }
    if (mod_mask & MOD_SHIFT) {
        g = 64;
    }
    if (mod_mask & MOD_ALT) {
        b = 64;
    }
    if (mod_mask & MOD_GUI) {
        r = 64;
        b = 64;
    }
    if (mod_mask & MOD_FN) {
        g = 32;
        b = 32;
    }
    neopixel_set_color(r, g, b);
}
