#pragma once

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define RGB565(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_RED     0xF800
#define COLOR_GREEN   0x07E0
#define COLOR_BLUE    0x001F
#define COLOR_YELLOW  0xFFE0
#define COLOR_CYAN    0x07FF
#define COLOR_MAGENTA 0xF81F

void ili9488_init(void);
void ili9488_set_rotation(uint8_t r);
uint16_t ili9488_width(void);
uint16_t ili9488_height(void);

void ili9488_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void ili9488_write_raw(const uint8_t *bytes, uint32_t count);
void ili9488_fill_color(uint16_t color, uint32_t count);

uint32_t ili9488_spi_baudrate(void);

void ili9488_pixel(uint16_t x, uint16_t y, uint16_t color);
void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9488_clear(uint16_t color);

#ifdef __cplusplus
}
#endif
