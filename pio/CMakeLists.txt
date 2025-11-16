add_library(pico_neopixel INTERFACE)

pico_generate_pio_header(pico_neopixel ${CMAKE_CURRENT_LIST_DIR}/../src/neopixel.pio)

target_include_directories(pico_neopixel INTERFACE ${CMAKE_CURRENT_BINARY_DIR})
