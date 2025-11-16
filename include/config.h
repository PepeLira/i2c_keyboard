#ifndef CONFIG_H
#define CONFIG_H

// GPIO Pin Definitions from keyboard_layout.json
#define I2C_SCL         0
#define I2C_SDA         1
#define POWER_BUTTON    29
#define NEOPIXEL_GPIO   28

// Power Button Configuration
#define DEBOUNCE_MS         20
#define POWER_HOLD_MS       3000   // Button must stay low for 3 seconds
#define BLINK_PERIOD_MS     1000   // Blink feedback every second
#define BLINK_ON_MS         500    // Half-second red pulse within the period
#define DISCHARGE_SAMPLE_MS 5      // How often to release pin to detect release

// NeoPixel Color Definition
typedef struct {
    uint32_t r;
    uint32_t g;
    uint32_t b;
} rgb_color_t;

// NeoPixel Configuration
#define NEOPIXEL_COUNT 1
#define DEFAULT_COLOR ((rgb_color_t){0, 5, 0})    // Green
#define SHUTDOWN_COLOR ((rgb_color_t){5, 0, 0})   // Red
#define MODIFIER_COLOR ((rgb_color_t){5, 5, 0})  // Orange

#endif
