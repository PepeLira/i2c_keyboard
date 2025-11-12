#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

void neopixel_init(void);
void neopixel_set_color(uint8_t r, uint8_t g, uint8_t b);
void neopixel_update_modifiers(uint8_t mod_mask);

#endif // NEOPIXEL_H
