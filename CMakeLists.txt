cmake_minimum_required(VERSION 3.13...3.27)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(3rd_party/pico-sdk/pico_sdk_init.cmake)

project(i2c_keyboard)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

# Add PIO subdirectory for NeoPixel
add_subdirectory(pio)

# Create executable
add_executable(i2c_keyboard
    src/main.c
    src/neopixel.c
    src/power_button.c
)

# Link pico standard library
target_link_libraries(i2c_keyboard
    pico_stdlib
    hardware_gpio
    hardware_pio
    pico_time
    pico_neopixel
)

# Include directories
target_include_directories(i2c_keyboard PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
)

# Enable USB output
pico_enable_stdio_usb(i2c_keyboard 1)
pico_enable_stdio_uart(i2c_keyboard 0)

# Create map/bin/hex files
pico_add_extra_outputs(i2c_keyboard)