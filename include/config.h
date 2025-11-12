#ifndef CONFIG_H
#define CONFIG_H

#include <stdint.h>

#define FW_VERSION_MAJOR 1
#define FW_VERSION_MINOR 0

#define I2C_ADDR_BASE 0x32
#define I2C_DEV_ID 0xB0

#define FIFO_SIZE 64
#define SCAN_RATE_HZ 1000
#define DEBOUNCE_MS 5

#define USE_INT_PIN 0
#define INT_GPIO 27

#define SDA_GPIO 0
#define SCL_GPIO 1

#define MATRIX_ROWS 7
#define MATRIX_COLS 6

static const uint8_t ROW_PINS[MATRIX_ROWS] = {2, 3, 4, 5, 6, 7, 8};
static const uint8_t COL_PINS[MATRIX_COLS] = {9, 10, 11, 12, 13, 14};

#define DISCRETE_KEY_COUNT 11
static const uint8_t DISCRETE_PINS[DISCRETE_KEY_COUNT] = {
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25
};

#define NEOPIXEL_GPIO 26
#define NEOPIXEL_COUNT 1

#define CURSOR_UP_INDEX 0
#define CURSOR_DOWN_INDEX 1
#define CURSOR_LEFT_INDEX 2
#define CURSOR_RIGHT_INDEX 3
#define CURSOR_CENTER_INDEX 4

#endif // CONFIG_H
