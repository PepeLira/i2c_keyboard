cmake_minimum_required(VERSION 3.13...3.27)

include(3rd_party/pico-sdk/pico_sdk_init.cmake)

project(i2c_keyboard C CXX ASM)

pico_sdk_init()

add_executable(i2c_keyboard
    src/main.c
    src/power_latch.c
    src/button.c
    src/led.c
    src/led_controller.c
    src/tick.c
    src/switch_tracker.c
    src/i2c_slave.c
    src/matrix_scanner.c
    src/fn_keys.c
    src/modifier_manager.c
    src/digital_mouse.c
    src/key_fifo.c
)

pico_generate_pio_header(i2c_keyboard ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_include_directories(i2c_keyboard PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(i2c_keyboard pico_stdlib hardware_pio hardware_timer hardware_i2c)

pico_add_extra_outputs(i2c_keyboard)

set_property(TARGET i2c_keyboard PROPERTY C_STANDARD 11)

add_library(switch_logic STATIC src/switch_tracker.c)
target_include_directories(switch_logic PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
set_property(TARGET switch_logic PROPERTY C_STANDARD 11)
