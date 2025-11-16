#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

typedef enum {
    COLOR_GREEN = 0,
    COLOR_RED = 1,
    COLOR_OFF = 2
} neopixel_color_t;

void neopixel_init(void);
void neopixel_set_color(uint32_t r, uint32_t g, uint32_t b);
void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void neopixel_off(void);

#endif
