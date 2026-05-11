cmake_minimum_required(VERSION 3.13)

set(PICO_SDK_PATH $ENV{HOME}/work/pico-sdk)
include(pico_sdk_import.cmake)

project(ili9488_pico C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(ili9488-pico
    src/main.c
    src/ili9488.c
    src/gfx.c
    src/font8x16.c
    src/backlight.c
)

target_include_directories(ili9488-pico PRIVATE src)

target_link_libraries(ili9488-pico
    pico_stdlib
    hardware_spi
    hardware_dma
    hardware_pwm
    hardware_gpio
)

pico_add_extra_outputs(ili9488-pico)
