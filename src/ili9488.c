#include "ili9488.h"

#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"
#include "hardware/gpio.h"

#define CMD_SLPOUT    0x11
#define CMD_DISPON    0x29
#define CMD_CASET     0x2A
#define CMD_PASET     0x2B
#define CMD_RAMWR     0x2C
#define CMD_MADCTL    0x36
#define CMD_PIXFMT    0x3A

#define PIXFMT_16BIT  0x55

#define SCRATCH_PIXELS 768
static uint8_t scratch[SCRATCH_PIXELS * 2];
static int dma_ch;
static uint16_t cur_w = ILI9488_PANEL_H;
static uint16_t cur_h = ILI9488_PANEL_W;

static inline void cs_lo(void) { gpio_put(ILI9488_PIN_CS, 0); }
static inline void cs_hi(void) { gpio_put(ILI9488_PIN_CS, 1); }
static inline void dc_cmd(void) { gpio_put(ILI9488_PIN_DC, 0); }
static inline void dc_data(void) { gpio_put(ILI9488_PIN_DC, 1); }

static void wr_cmd_data(uint8_t c, const uint8_t *d, size_t n) {
    dc_cmd();
    cs_lo();
    spi_write_blocking(ILI9488_SPI, &c, 1);
    if (n) {
        dc_data();
        spi_write_blocking(ILI9488_SPI, d, n);
    }
    cs_hi();
}

static void begin_pixels(void) {
    dc_cmd();
    cs_lo();
    uint8_t c = CMD_RAMWR;
    spi_write_blocking(ILI9488_SPI, &c, 1);
    dc_data();
}

static void end_pixels(void) {
    cs_hi();
}

static void pump_bytes(const uint8_t *buf, size_t n) {
    dma_channel_set_read_addr(dma_ch, buf, false);
    dma_channel_set_trans_count(dma_ch, n, true);
    dma_channel_wait_for_finish_blocking(dma_ch);
    while (spi_is_busy(ILI9488_SPI)) tight_loop_contents();
}

static void hw_reset(void) {
    gpio_put(ILI9488_PIN_RST, 1);
    sleep_ms(5);
    gpio_put(ILI9488_PIN_RST, 0);
    sleep_ms(20);
    gpio_put(ILI9488_PIN_RST, 1);
    sleep_ms(150);
}

static const uint8_t gamma_pos[] = {0x00,0x03,0x09,0x08,0x16,0x0A,0x3F,0x78,0x4C,0x09,0x0A,0x08,0x16,0x1A,0x0F};
static const uint8_t gamma_neg[] = {0x00,0x16,0x19,0x03,0x0F,0x05,0x32,0x45,0x46,0x04,0x0E,0x0D,0x35,0x37,0x0F};

static void panel_init(void) {
    wr_cmd_data(0xE0, gamma_pos, sizeof(gamma_pos));
    wr_cmd_data(0xE1, gamma_neg, sizeof(gamma_neg));
    wr_cmd_data(0xC0, (const uint8_t[]){0x17, 0x15}, 2);
    wr_cmd_data(0xC1, (const uint8_t[]){0x41}, 1);
    wr_cmd_data(0xC5, (const uint8_t[]){0x00, 0x12, 0x80}, 3);
    wr_cmd_data(CMD_PIXFMT, (const uint8_t[]){PIXFMT_16BIT}, 1);
    wr_cmd_data(0xB0, (const uint8_t[]){0x00}, 1);
    wr_cmd_data(0xB1, (const uint8_t[]){0xA0}, 1);
    wr_cmd_data(0xB4, (const uint8_t[]){0x02}, 1);
    wr_cmd_data(0xB6, (const uint8_t[]){0x02, 0x02, 0x3B}, 3);
    wr_cmd_data(0xB7, (const uint8_t[]){0xC6}, 1);
    wr_cmd_data(0xF7, (const uint8_t[]){0xA9, 0x51, 0x2C, 0x82}, 4);
    wr_cmd_data(CMD_SLPOUT, NULL, 0);
    sleep_ms(120);
    wr_cmd_data(CMD_DISPON, NULL, 0);
    sleep_ms(20);
}

void ili9488_init(void) {
    spi_init(ILI9488_SPI, ILI9488_SPI_HZ);
    spi_set_format(ILI9488_SPI, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_set_function(ILI9488_PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(ILI9488_PIN_MOSI, GPIO_FUNC_SPI);

    gpio_init(ILI9488_PIN_CS);  gpio_set_dir(ILI9488_PIN_CS, GPIO_OUT);  gpio_put(ILI9488_PIN_CS, 1);
    gpio_init(ILI9488_PIN_DC);  gpio_set_dir(ILI9488_PIN_DC, GPIO_OUT);  gpio_put(ILI9488_PIN_DC, 1);
    gpio_init(ILI9488_PIN_RST); gpio_set_dir(ILI9488_PIN_RST, GPIO_OUT); gpio_put(ILI9488_PIN_RST, 1);

    dma_ch = dma_claim_unused_channel(true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_ch);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_8);
    channel_config_set_dreq(&cfg, spi_get_dreq(ILI9488_SPI, true));
    channel_config_set_read_increment(&cfg, true);
    channel_config_set_write_increment(&cfg, false);
    dma_channel_configure(dma_ch, &cfg, &spi_get_hw(ILI9488_SPI)->dr, NULL, 0, false);

    hw_reset();
    panel_init();
    ili9488_set_rotation(ILI9488_DEFAULT_ROTATION);
    ili9488_clear(COLOR_BLACK);
}

void ili9488_set_rotation(uint8_t r) {
    static const uint8_t madctl[4] = {0x48, 0x28, 0x88, 0xE8};
    r &= 3;
    wr_cmd_data(CMD_MADCTL, &madctl[r], 1);
    if (r & 1) { cur_w = ILI9488_PANEL_H; cur_h = ILI9488_PANEL_W; }
    else       { cur_w = ILI9488_PANEL_W; cur_h = ILI9488_PANEL_H; }
}

uint16_t ili9488_width(void)  { return cur_w; }
uint16_t ili9488_height(void) { return cur_h; }

void ili9488_set_window(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t x1 = x + w - 1;
    uint16_t y1 = y + h - 1;
    uint8_t a[4];
    a[0] = x >> 8;  a[1] = x & 0xFF;  a[2] = x1 >> 8; a[3] = x1 & 0xFF;
    wr_cmd_data(CMD_CASET, a, 4);
    a[0] = y >> 8;  a[1] = y & 0xFF;  a[2] = y1 >> 8; a[3] = y1 & 0xFF;
    wr_cmd_data(CMD_PASET, a, 4);
}

void ili9488_write_raw(const uint8_t *bytes, uint32_t count) {
    begin_pixels();
    pump_bytes(bytes, count);
    end_pixels();
}

uint32_t ili9488_spi_baudrate(void) {
    return spi_get_baudrate(ILI9488_SPI);
}

void ili9488_fill_color(uint16_t color, uint32_t count) {
    uint8_t hi = (uint8_t)(color >> 8);
    uint8_t lo = (uint8_t)color;
    for (uint32_t i = 0; i < SCRATCH_PIXELS; i++) {
        scratch[i * 2]     = hi;
        scratch[i * 2 + 1] = lo;
    }
    begin_pixels();
    while (count) {
        uint32_t n = count > SCRATCH_PIXELS ? SCRATCH_PIXELS : count;
        pump_bytes(scratch, n * 2);
        count -= n;
    }
    end_pixels();
}

void ili9488_pixel(uint16_t x, uint16_t y, uint16_t color) {
    if (x >= cur_w || y >= cur_h) return;
    ili9488_set_window(x, y, 1, 1);
    uint8_t b[2] = { (uint8_t)(color >> 8), (uint8_t)color };
    begin_pixels();
    spi_write_blocking(ILI9488_SPI, b, 2);
    end_pixels();
}

void ili9488_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    if (x >= cur_w || y >= cur_h) return;
    if (x + w > cur_w) w = cur_w - x;
    if (y + h > cur_h) h = cur_h - y;
    ili9488_set_window(x, y, w, h);
    ili9488_fill_color(color, (uint32_t)w * h);
}

void ili9488_clear(uint16_t color) {
    ili9488_fill_rect(0, 0, cur_w, cur_h, color);
}
