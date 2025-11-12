cmake_minimum_required(VERSION 3.13...3.27)

# initialize pico-sdk from submodule
# note: this must happen before project()
include(3rd_party/pico-sdk/pico_sdk_init.cmake)

project(i2c_keyboard C CXX ASM)

pico_sdk_init()

set(SRC_DIR ${CMAKE_CURRENT_LIST_DIR}/src)

add_executable(i2c_keyboard
    ${SRC_DIR}/main.c
    ${SRC_DIR}/fifo.c
    ${SRC_DIR}/matrix_scan.c
    ${SRC_DIR}/discrete_keys.c
    ${SRC_DIR}/i2c_slave.c
    ${SRC_DIR}/neopixel.c
)

pico_generate_pio_header(i2c_keyboard ${SRC_DIR}/ws2812.pio)

target_include_directories(i2c_keyboard PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include)

target_link_libraries(i2c_keyboard
    pico_stdlib
    hardware_i2c
    hardware_pio
    hardware_irq
)

pico_add_extra_outputs(i2c_keyboard)
