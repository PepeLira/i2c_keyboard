cmake_minimum_required(VERSION 3.13)

include(3rd_party/pico-sdk/pico_sdk_init.cmake)

project(i2c_keyboard C CXX ASM)

pico_sdk_init()

add_executable(i2c_keyboard
    src/config.c
    src/discrete_keys.c
    src/fifo.c
    src/i2c_slave.c
    src/layout_default.c
    src/main.c
    src/matrix_scan.c
    src/neopixel.c
)

pico_generate_pio_header(
    i2c_keyboard
    ${CMAKE_CURRENT_LIST_DIR}/pio/neopixel.pio
    OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/pio
)

target_include_directories(i2c_keyboard PRIVATE
    include
    ${CMAKE_CURRENT_BINARY_DIR}
)

target_link_libraries(i2c_keyboard
    pico_stdlib
    hardware_gpio
    hardware_i2c
    hardware_pio
    hardware_clocks
)

pico_enable_stdio_usb(i2c_keyboard 1)
pico_enable_stdio_uart(i2c_keyboard 0)

pico_add_extra_outputs(i2c_keyboard)
