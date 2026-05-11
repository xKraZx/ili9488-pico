#include "backlight.h"
#include "ili9488_config.h"

#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include "hardware/clocks.h"

#define BL_WRAP 0xFFFFu
#define BL_HZ   1000u

static uint slice;
static uint chan;

void backlight_init(void) {
    gpio_set_function(ILI9488_PIN_LED, GPIO_FUNC_PWM);
    slice = pwm_gpio_to_slice_num(ILI9488_PIN_LED);
    chan  = pwm_gpio_to_channel(ILI9488_PIN_LED);

    uint32_t sys_hz = clock_get_hz(clk_sys);
    float div = (float)sys_hz / ((float)BL_HZ * (BL_WRAP + 1));
    if (div < 1.0f) div = 1.0f;

    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_clkdiv(&cfg, div);
    pwm_config_set_wrap(&cfg, BL_WRAP);
    pwm_init(slice, &cfg, true);

    backlight_set(100);
}

void backlight_set_raw(uint16_t level) {
    pwm_set_chan_level(slice, chan, level);
}

void backlight_set(uint8_t percent) {
    if (percent > 100) percent = 100;
    uint32_t level = ((uint32_t)percent * BL_WRAP) / 100u;
    pwm_set_chan_level(slice, chan, (uint16_t)level);
}
