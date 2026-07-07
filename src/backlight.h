#pragma once

#include <stdint.h>

void backlight_init(void);
void backlight_set_raw(uint16_t level);
void backlight_set(uint8_t percent);
uint16_t backlight_get_raw(void);
