#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define I2C_ADDR_BASE   0x32
#define FIFO_SIZE       64
#define SCAN_RATE_HZ    1000
#define DEBOUNCE_US     7000

#define USE_INT_PIN     1
#define INT_GPIO        27

#define SDA_GPIO        0
#define SCL_GPIO        1

#define ROW_COUNT       6
#define COL_COUNT       7

#define DISCRETE_COUNT  11

extern const uint8_t ROW_PINS[ROW_COUNT];
extern const uint8_t COL_PINS[COL_COUNT];
extern const uint8_t DISCRETE_PINS[DISCRETE_COUNT];

#define NEOPIXEL_GPIO   28
#define NEOPIXEL_COUNT  1

#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0

#endif // CONFIG_H
