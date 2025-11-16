#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/gpio.h"
#include "hardware/pio.h"

#include "config.h"
#include "neopixel.h"
#include "power_button.h"

// Binary info for UF2 file
bi_decl(bi_program_description("RP2040 I2C Keyboard with Power Button and NeoPixel"));
bi_decl(bi_program_version_string("1.0.0"));

void init_gpios(void) {
    // Initialize power button as pull-up input
    gpio_init(POWER_BUTTON);
    gpio_set_dir(POWER_BUTTON, GPIO_IN);
    // Use pull-up - when button is pressed, it goes to GND (LOW), otherwise HIGH
    gpio_pull_up(POWER_BUTTON);
    
    // All other GPIO initialization handled by respective modules
}

int main(void) {
    stdio_init_all();
    
    // Wait a moment for USB to stabilize if connected
    sleep_ms(100);
    
    printf("RP2040 Keyboard Firmware Starting...\n");
    printf("Power Button GPIO: %d\n", POWER_BUTTON);
    printf("NeoPixel GPIO: %d\n", NEOPIXEL_GPIO);
    
    // Initialize all GPIO pins
    init_gpios();
    
    // Initialize NeoPixel LED
    neopixel_init();
    printf("NeoPixel initialized\n");
    
    // Set default light green color (GRB format: G=100, R=0, B=0 for light green)
    neopixel_set_color(5, 0, 0);  // Light green
    printf("NeoPixel set to light green\n");
    
    // Initialize power button handler
    power_button_init();
    printf("Power button handler initialized\n");
    
    printf("Firmware ready. Waiting for power button presses...\n");
    
    // Main loop - just keep monitoring power button
    while (1) {
        power_button_update();
        sleep_ms(1);  // 1ms update rate
    }
    
    return 0;
}
