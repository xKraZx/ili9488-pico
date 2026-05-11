# ili9488-pico

A small, fast ILI9488 driver for the Raspberry Pi Pico (RP2040), written in C
against the official `pico-sdk`. Drives the 320×480 TFT modules commonly sold
as 3.5" (e.g. LCDWIKI MSP3520) or 4" — both use the same ILI9488 controller
and pin layout. Uses hardware SPI + DMA for pixel streaming, PWM for
backlight dimming.

Written by Claude Opus 4.7 (Anthropic) under direction. Tested on real
Pico (RP2040) hardware with a 4" ILI9488 module.

## Performance

Measured on a real Pico (RP2040 @ 125 MHz, default `clk_peri`), ILI9488 at
16-bit RGB565, 62.5 MHz hardware SPI, DMA streaming, run-based rasterization
with a 256-entry per-glyph-row pixel lookup table.

```
Full-screen text repaint  (480×320, 60×20 cell grid)  ≈ 19 FPS
Single-line redraw        (60 chars, font 8×16)       ≈ 390 line redraws/s
Single-cell swap          (cursor blink at one cell)  ≈ several thousand/s
```

Why these numbers:

* **Full-screen text ≈ 19 FPS.** Frame is 480×320×2 = 307,200 bytes. At
  62.5 MHz hardware SPI that's ≈ 39 ms of DMA, plus ≈ 13 ms of CPU
  rasterization and per-row window setup, ≈ 52 ms/frame. We measured 19
  FPS = 53 ms. We're essentially pinned to the SPI bandwidth ceiling.

* **Partial line redraws ≈ 390 line/s.** One line is 60×8×16 = 7,680
  pixels = 15,360 bytes. At 62.5 MHz that's ≈ 1.97 ms of DMA. Add ~0.6
  ms rasterization + setup → ≈ 2.56 ms/line. 1 / 2.56 ms ≈ 390. This is
  the realistic "text editor typing" case: keystrokes only repaint the
  changed line, so the responsiveness ceiling is ~390 Hz — well past
  human-perceptible.

The journey to 19 FPS was incremental:

| Iteration                                     | FPS |
| --------------------------------------------- | --- |
| Initial: 18-bit RGB666, bit-by-bit raster     |  4  |
| + run-based rasterization (whole row / DMA)   | 10  |
| + 256-entry RGB lookup table for glyph rows   | 13  |
| + 16-bit RGB565 mode (PIXFMT `0x55`, 2 B/px)  | 19  |

## Hardware wiring

Default pin mapping (edit `src/ili9488_config.h` to change):

```
LCD pin   Pico
-------   ----
VCC       3V3
GND       GND
SCK       GP18  (SPI0 SCK)
SDI/MOSI  GP19  (SPI0 TX)
SDO/MISO  GP16  (SPI0 RX, unused but configured)
CS        GP17
DC/RS     GP20
RESET     GP21
LED       GP22  (PWM dimmable)
```

## Build

Requires `pico-sdk` at `~/work/pico-sdk` and an `arm-none-eabi` toolchain.

```
mkdir -p build && cd build
cmake -DPICO_BOARD=pico ..
make -j
```

Output: `build/ili9488-pico.uf2`.

## Flashing

Hold BOOTSEL on the Pico, press RESET (or replug USB). The Pico mounts as a
mass-storage volume labeled `RPI-RP2`. Copy `build/ili9488-pico.uf2` onto it;
the Pico auto-reboots into the new firmware.

## API surface

### Display driver — `ili9488.h`

```c
void ili9488_init(void);
void ili9488_set_rotation(uint8_t r);          // r in 0..3
uint16_t ili9488_width(void);
uint16_t ili9488_height(void);

void ili9488_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void ili9488_write_raw(const uint8_t *bytes, uint32_t count);
void ili9488_fill_color(uint16_t color, uint32_t count);

void ili9488_pixel(uint16_t x, uint16_t y, uint16_t color);
void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ili9488_clear(uint16_t color);

uint32_t ili9488_spi_baudrate(void);
```

Colors are RGB565 `uint16_t`. The `RGB565(r, g, b)` macro and a small set of
named constants (`COLOR_WHITE`, `COLOR_RED`, …) are in the header.

### Graphics primitives — `gfx.h`

```c
void gfx_hline / vline / line / rect;
void gfx_circle / gfx_fill_circle;
void gfx_char(x, y, ch, fg, bg);
void gfx_text(x, y, str, fg, bg);              // handles \n
```

Text uses an 8×16 bitmap font. Each `gfx_text` call rasterizes the entire run
into a pre-converted RGB565 buffer using a 256-entry pixel lookup table, then
streams it as one DMA blit.

### Backlight — `backlight.h`

```c
void backlight_init(void);
void backlight_set(uint8_t percent);           // 0..100
void backlight_set_raw(uint16_t level);        // 0..65535
```

1 kHz PWM, 16-bit resolution.

## Using in your own project

Plain pico-sdk C, no external dependencies. Clone next to your project:

```
git clone https://github.com/jackdoe/ili9488-pico
```

Then point your `add_executable` at the files in `ili9488-pico/src/`,
include `ili9488-pico/src` on the include path, and link the four
hardware libraries the driver needs:

```cmake
add_executable(my_app
    main.c
    ili9488-pico/src/ili9488.c
    ili9488-pico/src/gfx.c
    ili9488-pico/src/font8x16.c
    ili9488-pico/src/backlight.c
)

target_include_directories(my_app PRIVATE ili9488-pico/src)

target_link_libraries(my_app
    pico_stdlib
    hardware_spi
    hardware_dma
    hardware_pwm
    hardware_gpio
)

pico_add_extra_outputs(my_app)
```

Edit `ili9488-pico/src/ili9488_config.h` for your pin assignments and
(if needed) the SPI peripheral and clock — or copy it into your own
project root and shadow it via an earlier include directory if you'd
rather not modify the cloned tree.

Use it:

```c
#include "ili9488.h"
#include "gfx.h"
#include "backlight.h"

int main(void) {
    backlight_init();
    ili9488_init();
    backlight_set(80);

    ili9488_clear(COLOR_BLACK);
    gfx_text(10, 10, "Hello, world", COLOR_WHITE, COLOR_BLACK);
    while (1) tight_loop_contents();
}
```

If you don't need backlight dimming, drop `backlight.c` from the source
list and wire the LED pin to 3V3 — `hardware_pwm` can come off the link
list too.

If you don't need shapes/text, drop `gfx.c` and `font8x16.c` — the
ili9488 module alone gives you raw pixel/rect/fill/window control.

## Project layout

```
.
├── CMakeLists.txt
├── pico_sdk_import.cmake
└── src/
    ├── ili9488_config.h       pins, SPI clock, panel size
    ├── ili9488.h / .c         init, DMA blit, rotation, window
    ├── gfx.h  / .c            lines, circles, text rendering
    ├── font8x16.c             96-glyph ASCII bitmap font
    ├── backlight.h / .c       PWM dimming
    └── main.c                 demo + benchmarks
```

## Notes

* SPI runs at 62.5 MHz (the RP2040 hardware ceiling at default `clk_peri =
  125 MHz`). This is above the ILI9488 datasheet's `t_wc = 50 ns` write spec
  (20 MHz) but well within the silicon's actual capability on short-ribbon
  modules. If your panel shows corruption, dial `ILI9488_SPI_HZ` down in
  `ili9488_config.h` (try 31250000 or 20000000).

* Pixel format is 16-bit RGB565 (PIXFMT `0x55`). The datasheet also lists
  18-bit RGB666 and 3-bit RGB111; in practice no open implementation has
  3-bit working over 4-wire SPI on common modules — RGB565 is the sweet
  spot.

* The rotation demo cycles through all 4 MADCTL orientations. Benchmarks
  run in landscape (rotation 1 = 480×320).
