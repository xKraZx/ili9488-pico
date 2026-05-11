#include <stdio.h>

#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/clocks.h"

#include "ili9488.h"
#include "gfx.h"
#include "backlight.h"

static void show_clocks(void) {
    char buf[64];
    ili9488_clear(COLOR_BLACK);
    gfx_text(8,  16, "CLOCK / SPI STATUS", COLOR_CYAN, COLOR_BLACK);
    snprintf(buf, sizeof(buf), "clk_sys:  %lu Hz", (unsigned long)clock_get_hz(clk_sys));
    gfx_text(8,  56, buf, COLOR_WHITE, COLOR_BLACK);
    snprintf(buf, sizeof(buf), "clk_peri: %lu Hz", (unsigned long)clock_get_hz(clk_peri));
    gfx_text(8,  80, buf, COLOR_WHITE, COLOR_BLACK);
    snprintf(buf, sizeof(buf), "spi baud: %lu Hz", (unsigned long)ili9488_spi_baudrate());
    gfx_text(8, 104, buf, COLOR_GREEN, COLOR_BLACK);
    sleep_ms(3000);
}

static void demo_shapes(void) {
    uint16_t w = ili9488_width();
    uint16_t h = ili9488_height();

    ili9488_clear(COLOR_BLACK);

    gfx_rect(0, 0, w, h, COLOR_WHITE);
    gfx_line(0, 0, w - 1, h - 1, COLOR_RED);
    gfx_line(w - 1, 0, 0, h - 1, COLOR_BLUE);

    gfx_fill_circle(w / 2, h / 2, 60, COLOR_GREEN);
    gfx_circle(w / 2, h / 2, 80, COLOR_YELLOW);

    ili9488_fill_rect(20, 20, 80, 40, COLOR_MAGENTA);
    ili9488_fill_rect(w - 100, 20, 80, 40, COLOR_CYAN);

    gfx_text(20, h - 40, "ILI9488 @ Pico", COLOR_WHITE, COLOR_BLACK);
    gfx_text(20, h - 24, "RGB565 / DMA / PWM", COLOR_YELLOW, COLOR_BLACK);
}

static void bench_fullscreen_text(void) {
    int cols = ili9488_width() / GFX_FONT_W;
    int rows = ili9488_height() / GFX_FONT_H;
    int per_frame = cols * rows;
    const int frames = 16;
    char line[64];
    if (cols > (int)sizeof(line) - 1) cols = sizeof(line) - 1;

    ili9488_clear(COLOR_BLACK);
    uint64_t t0 = time_us_64();
    for (int f = 0; f < frames; f++) {
        char c = (char)('!' + (f * 17) % 90);
        for (int i = 0; i < cols; i++) line[i] = c;
        line[cols] = '\0';
        for (int r = 0; r < rows; r++)
            gfx_text(0, r * GFX_FONT_H, line, COLOR_WHITE, COLOR_BLACK);
    }
    uint64_t dt = time_us_64() - t0;

    int fps_x10 = (int)((uint64_t)frames * 10000000ULL / dt);
    int cps     = (int)((uint64_t)frames * per_frame * 1000000ULL / dt);

    char buf[64];
    ili9488_clear(COLOR_BLACK);
    gfx_text(8,  16, "FULL-SCREEN TEXT BENCH", COLOR_CYAN,   COLOR_BLACK);
    snprintf(buf, sizeof(buf), "grid:      %d x %d (%d cells)", cols, rows, per_frame);
    gfx_text(8,  56, buf, COLOR_WHITE,  COLOR_BLACK);
    snprintf(buf, sizeof(buf), "frames:    %d", frames);
    gfx_text(8,  80, buf, COLOR_WHITE,  COLOR_BLACK);
    snprintf(buf, sizeof(buf), "elapsed:   %d ms", (int)(dt / 1000));
    gfx_text(8, 104, buf, COLOR_WHITE,  COLOR_BLACK);
    snprintf(buf, sizeof(buf), "FPS:       %d.%d", fps_x10 / 10, fps_x10 % 10);
    gfx_text(8, 136, buf, COLOR_GREEN,  COLOR_BLACK);
    snprintf(buf, sizeof(buf), "chars/sec: %d", cps);
    gfx_text(8, 160, buf, COLOR_YELLOW, COLOR_BLACK);

    sleep_ms(4000);
}

static void bench_single_char_swap(void) {
    ili9488_clear(COLOR_BLACK);
    gfx_text(8,  16, "SINGLE-CHAR SWAP RATE",   COLOR_CYAN,  COLOR_BLACK);
    gfx_text(8,  40, "(cursor blink at fixed cell)", COLOR_WHITE, COLOR_BLACK);

    const int iters = 4000;
    uint64_t t0 = time_us_64();
    for (int i = 0; i < iters; i++) {
        char c = (i & 1) ? '_' : ' ';
        gfx_char(240, 100, c, COLOR_GREEN, COLOR_BLACK);
    }
    uint64_t dt = time_us_64() - t0;

    int cps = (int)((uint64_t)iters * 1000000ULL / dt);
    char buf[64];
    snprintf(buf, sizeof(buf), "iters:    %d", iters);
    gfx_text(8, 140, buf, COLOR_WHITE,  COLOR_BLACK);
    snprintf(buf, sizeof(buf), "elapsed:  %d ms", (int)(dt / 1000));
    gfx_text(8, 164, buf, COLOR_WHITE,  COLOR_BLACK);
    snprintf(buf, sizeof(buf), "swaps/s:  %d", cps);
    gfx_text(8, 188, buf, COLOR_YELLOW, COLOR_BLACK);

    gfx_text(8, 230, "visible blink ->", COLOR_WHITE, COLOR_BLACK);
    for (int i = 0; i < 8; i++) {
        char c = (i & 1) ? '_' : ' ';
        gfx_char(200, 230, c, COLOR_GREEN, COLOR_BLACK);
        sleep_ms(250);
    }
    sleep_ms(1500);
}

static void bench_partial_typing(void) {
    int cols = ili9488_width() / GFX_FONT_W;
    int rows = ili9488_height() / GFX_FONT_H;
    char line[64];
    if (cols > (int)sizeof(line) - 1) cols = sizeof(line) - 1;

    ili9488_clear(COLOR_BLACK);
    for (int r = 0; r < rows; r++) {
        for (int i = 0; i < cols; i++) line[i] = (char)('a' + (r * 7 + i * 3) % 26);
        line[cols] = '\0';
        gfx_text(0, r * GFX_FONT_H, line, COLOR_WHITE, COLOR_BLACK);
    }

    int typing_row = rows / 2 + 1;
    int typing_y = typing_row * GFX_FONT_H;

    const int redraws = 500;
    uint64_t t0 = time_us_64();
    for (int f = 0; f < redraws; f++) {
        for (int i = 0; i < cols; i++) line[i] = (char)('A' + (f * 7 + i * 3) % 26);
        line[cols] = '\0';
        gfx_text(0, typing_y, line, COLOR_YELLOW, COLOR_BLACK);
    }
    uint64_t dt = time_us_64() - t0;

    int line_fps_x10 = (int)((uint64_t)redraws * 10000000ULL / dt);

    char buf[64];
    snprintf(buf, sizeof(buf), " PARTIAL LINE REDRAW ");
    gfx_text(0, 0, buf, COLOR_BLACK, COLOR_CYAN);
    snprintf(buf, sizeof(buf), " %d redraws in %d ms ", redraws, (int)(dt / 1000));
    gfx_text(0, GFX_FONT_H, buf, COLOR_BLACK, COLOR_WHITE);
    snprintf(buf, sizeof(buf), " line FPS: %d.%d ", line_fps_x10 / 10, line_fps_x10 % 10);
    gfx_text(0, GFX_FONT_H * 2, buf, COLOR_BLACK, COLOR_GREEN);

    gfx_text(0, GFX_FONT_H * 4, "watch the highlighted line ->", COLOR_WHITE, COLOR_BLACK);
    for (int f = 0; f < 12; f++) {
        for (int i = 0; i < cols; i++) line[i] = (char)('A' + (f * 5 + i * 2) % 26);
        line[cols] = '\0';
        gfx_text(0, typing_y, line, (f & 1) ? COLOR_YELLOW : COLOR_GREEN, COLOR_BLACK);
        sleep_ms(250);
    }
    sleep_ms(2000);
}

static void demo_editor_sim(void) {
    int cols = ili9488_width() / GFX_FONT_W;
    int rows = ili9488_height() / GFX_FONT_H;
    int total = cols * rows;

    ili9488_clear(COLOR_BLACK);
    uint64_t t0 = time_us_64();
    for (int i = 0; i < total; i++) {
        int r  = i / cols;
        int co = i % cols;
        char ch = (char)('A' + (i * 7 + r) % 26);
        gfx_char(co * GFX_FONT_W, r * GFX_FONT_H, ch, COLOR_GREEN, COLOR_BLACK);
    }
    uint64_t dt = time_us_64() - t0;
    int cps = (int)((uint64_t)total * 1000000ULL / dt);

    char buf[64];
    snprintf(buf, sizeof(buf), " typed %d in %d ms ", total, (int)(dt / 1000));
    gfx_text(0, 0,  buf, COLOR_BLACK, COLOR_YELLOW);
    snprintf(buf, sizeof(buf), " %d cps ", cps);
    gfx_text(0, 16, buf, COLOR_BLACK, COLOR_GREEN);
    sleep_ms(3000);
}

static void demo_brightness_fade(void) {
    for (int p = 100; p >= 10; p -= 5) { backlight_set(p); sleep_ms(30); }
    for (int p = 10;  p <= 100; p += 5) { backlight_set(p); sleep_ms(30); }
}

int main(void) {
    backlight_init();
    ili9488_init();
    show_clocks();

    while (true) {
        for (uint8_t r = 0; r < 4; r++) {
            ili9488_set_rotation(r);
            demo_shapes();
            sleep_ms(1500);
        }
        ili9488_set_rotation(1);
        bench_fullscreen_text();
        bench_partial_typing();
        bench_single_char_swap();
        demo_editor_sim();
        demo_brightness_fade();
    }
}
