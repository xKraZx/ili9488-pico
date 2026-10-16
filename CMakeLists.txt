add_library(pico_ili9488
    src/ili9488.c
    src/gfx.c
    src/font8x16.c
    src/backlight.c
)

target_include_directories(pico_ili9488 PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(pico_ili9488 PUBLIC
    pico_stdlib
    hardware_spi
    hardware_dma
    hardware_pwm
    hardware_gpio
)
