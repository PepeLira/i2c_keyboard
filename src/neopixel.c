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
    neopixel_offset = pio_add_program(neopixel_pio, &neopixel_program);
    neopixel_sm = pio_claim_unused_sm(neopixel_pio, true);

    pio_sm_config config = neopixel_program_get_default_config(neopixel_offset);
    sm_config_set_sideset_pins(&config, NEOPIXEL_GPIO);
    sm_config_set_out_shift(&config, false, true, 24);
    sm_config_set_fifo_join(&config, PIO_FIFO_JOIN_TX);

    float div = (float)clock_get_hz(clk_sys) / 800000.0f;
    sm_config_set_clkdiv(&config, div);

    pio_gpio_init(neopixel_pio, NEOPIXEL_GPIO);
    pio_sm_set_consecutive_pindirs(neopixel_pio, neopixel_sm, NEOPIXEL_GPIO, 1, true);
    pio_sm_init(neopixel_pio, neopixel_sm, neopixel_offset, &config);
    pio_sm_set_enabled(neopixel_pio, neopixel_sm, true);
    neopixel_set_rgb(0, 0, 0);
}

void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    if (neopixel_sm < 0) {
        return;
    }
    uint32_t value = ((uint32_t)g << 16) | ((uint32_t)r << 8) | b;
    pio_sm_put_blocking(neopixel_pio, neopixel_sm, value << 8u);
}
