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
)

pico_generate_pio_header(i2c_keyboard ${CMAKE_CURRENT_LIST_DIR}/src/ws2812.pio)

target_include_directories(i2c_keyboard PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src)

target_link_libraries(i2c_keyboard pico_stdlib hardware_pio hardware_timer)

pico_add_extra_outputs(i2c_keyboard)

set_property(TARGET i2c_keyboard PROPERTY C_STANDARD 11)

enable_testing()

add_library(switch_logic STATIC src/switch_tracker.c)
target_include_directories(switch_logic PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src)
set_property(TARGET switch_logic PROPERTY C_STANDARD 11)

add_executable(switch_tracker_test tests/switch_tracker_test.c)
target_link_libraries(switch_tracker_test switch_logic)
set_property(TARGET switch_tracker_test PROPERTY C_STANDARD 11)

add_test(NAME switch_tracker_test COMMAND switch_tracker_test)
