#ifndef NEOPIXEL_H
#define NEOPIXEL_H

#include <stdint.h>

#include "config.h"
#include "event.h"
#include "power_fsm.h"

void neopixel_init(void);
void neopixel_set_color(rgb_color_t color);
void neopixel_set_rgb(uint8_t r, uint8_t g, uint8_t b);
void neopixel_off(void);

void neopixel_handle_event(const event_t *event);

#endif
