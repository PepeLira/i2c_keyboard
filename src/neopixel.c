#include "neopixel.h"
#include "config.h"
#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "neopixel.pio.h"

// NeoPixel state
static PIO pio_instance = pio0;
static uint state_machine = 0;

// WS2812 color order is GRB
// We'll send 24-bit values (8-bit for each color)

void neopixel_init(void) {
    // Initialize PIO for NeoPixel control
    uint offset = pio_add_program(pio_instance, &ws2812_program);
    state_machine = pio_claim_unused_sm(pio_instance, true);
    
    // Initialize with 800kHz frequency for WS2812B
    ws2812_program_init(pio_instance, state_machine, offset, NEOPIXEL_GPIO, 800000.0f, false);
}

void neopixel_set_color(rgb_color_t color) {
    // Combine RGB into 24-bit value (GRB format for WS2812)
    // The data is shifted out MSB first
    uint32_t color_f = ((uint32_t)color.g << 16) | 
                     ((uint32_t)color.r << 8) | 
                     (uint32_t)color.b;
    
    pio_sm_put_blocking(pio_instance, state_machine, color_f << 8);
}

void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b) {
    // Convenience wrapper - same as set_color but parameter order matches RGB
    neopixel_set_color((rgb_color_t){r, g, b});
}

void neopixel_off(void) {
    neopixel_set_color((rgb_color_t){0, 0, 0});
}
