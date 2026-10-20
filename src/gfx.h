#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define GFX_FONT_W 8
#define GFX_FONT_H 16

extern const uint8_t font8x16[96 * 16];

void gfx_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color);
void gfx_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color);
void gfx_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void gfx_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void gfx_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);
void gfx_fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color);

void gfx_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);
void gfx_text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg);

#ifdef __cplusplus
}
#endif
