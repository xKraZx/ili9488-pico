#include "gfx.h"

#include <string.h>
#include <stdbool.h>

#include "pico/stdlib.h"
#include "ili9488.h"

#define MAX_RUN_W ILI9488_PANEL_H
#define PIX_BYTES_PER_GLYPH_ROW (GFX_FONT_W * 2)

static uint8_t run_buf[MAX_RUN_W * GFX_FONT_H * 2];
static uint8_t byte_to_pixels[256][PIX_BYTES_PER_GLYPH_ROW];
static uint16_t cached_fg = 0x0001;
static uint16_t cached_bg = 0x0001;
static bool table_valid = false;

static inline int16_t abs16(int16_t v) { return v < 0 ? -v : v; }

static void rebuild_table(uint16_t fg, uint16_t bg) {
    uint8_t fg_hi = (uint8_t)(fg >> 8), fg_lo = (uint8_t)fg;
    uint8_t bg_hi = (uint8_t)(bg >> 8), bg_lo = (uint8_t)bg;
    for (int b = 0; b < 256; b++) {
        uint8_t *p = byte_to_pixels[b];
        for (int col = 0; col < GFX_FONT_W; col++) {
            if (b & (0x80 >> col)) { *p++ = fg_hi; *p++ = fg_lo; }
            else                   { *p++ = bg_hi; *p++ = bg_lo; }
        }
    }
    cached_fg = fg;
    cached_bg = bg;
    table_valid = true;
}

void gfx_hline(uint16_t x, uint16_t y, uint16_t w, uint16_t color) {
    ili9488_fill_rect(x, y, w, 1, color);
}

void gfx_vline(uint16_t x, uint16_t y, uint16_t h, uint16_t color) {
    ili9488_fill_rect(x, y, 1, h, color);
}

void gfx_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {
    if (y0 == y1) { gfx_hline(x0 < x1 ? x0 : x1, y0, abs16(x1 - x0) + 1, color); return; }
    if (x0 == x1) { gfx_vline(x0, y0 < y1 ? y0 : y1, abs16(y1 - y0) + 1, color); return; }
    int16_t dx = abs16(x1 - x0), sx = x0 < x1 ? 1 : -1;
    int16_t dy = -abs16(y1 - y0), sy = y0 < y1 ? 1 : -1;
    int16_t err = dx + dy;
    for (;;) {
        ili9488_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;
        int16_t e2 = err << 1;
        if (e2 >= dy) { err += dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

void gfx_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    gfx_hline(x, y, w, color);
    gfx_hline(x, y + h - 1, w, color);
    gfx_vline(x, y, h, color);
    gfx_vline(x + w - 1, y, h, color);
}

void gfx_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
        ili9488_pixel(cx + x, cy + y, color);
        ili9488_pixel(cx - x, cy + y, color);
        ili9488_pixel(cx + x, cy - y, color);
        ili9488_pixel(cx - x, cy - y, color);
        ili9488_pixel(cx + y, cy + x, color);
        ili9488_pixel(cx - y, cy + x, color);
        ili9488_pixel(cx + y, cy - x, color);
        ili9488_pixel(cx - y, cy - x, color);
        x++;
        if (d > 0) { y--; d += 4 * (x - y) + 10; }
        else d += 4 * x + 6;
    }
}

void gfx_fill_circle(int16_t cx, int16_t cy, int16_t r, uint16_t color) {
    int16_t x = 0, y = r, d = 3 - 2 * r;
    while (y >= x) {
        gfx_hline(cx - x, cy + y, 2 * x + 1, color);
        gfx_hline(cx - x, cy - y, 2 * x + 1, color);
        gfx_hline(cx - y, cy + x, 2 * y + 1, color);
        gfx_hline(cx - y, cy - x, 2 * y + 1, color);
        x++;
        if (d > 0) { y--; d += 4 * (x - y) + 10; }
        else d += 4 * x + 6;
    }
}

static void render_run(uint16_t x, uint16_t y, const char *s, int n, uint16_t fg, uint16_t bg) {
    if (n <= 0) return;
    uint16_t sw = ili9488_width();
    uint16_t sh = ili9488_height();
    if (x >= sw || y >= sh) return;
    int max_n = (sw - x) / GFX_FONT_W;
    if (n > max_n) n = max_n;
    if (n <= 0) return;

    if (!table_valid || fg != cached_fg || bg != cached_bg) rebuild_table(fg, bg);

    int width_px = n * GFX_FONT_W;
    int row_stride = width_px * 2;
    for (int row = 0; row < GFX_FONT_H; row++) {
        uint8_t *out = run_buf + row * row_stride;
        for (int ci = 0; ci < n; ci++) {
            uint8_t ch = (uint8_t)s[ci];
            if (ch < 0x20 || ch > 0x7F) ch = '?';
            uint8_t bits = font8x16[(ch - 0x20) * 16 + row];
            memcpy(out, byte_to_pixels[bits], PIX_BYTES_PER_GLYPH_ROW);
            out += PIX_BYTES_PER_GLYPH_ROW;
        }
    }

    ili9488_set_window(x, y, width_px, GFX_FONT_H);
    ili9488_write_raw(run_buf, (uint32_t)width_px * GFX_FONT_H * 2);
}

void gfx_char(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg) {
    render_run(x, y, &c, 1, fg, bg);
}

void gfx_text(uint16_t x, uint16_t y, const char *s, uint16_t fg, uint16_t bg) {
    uint16_t cy = y;
    while (*s) {
        const char *line = s;
        while (*s && *s != '\n') s++;
        int n = (int)(s - line);
        if (n > 0) render_run(x, cy, line, n, fg, bg);
        if (*s == '\n') { cy += GFX_FONT_H; s++; }
    }
}
