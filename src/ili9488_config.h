#pragma once

#include "hardware/spi.h"

#define ILI9488_SPI         spi0
#define ILI9488_SPI_HZ      62500000

#define ILI9488_PIN_SCK     18
#define ILI9488_PIN_MOSI    19
#define ILI9488_PIN_MISO    16
#define ILI9488_PIN_CS      17
#define ILI9488_PIN_DC      20
#define ILI9488_PIN_RST     21
#define ILI9488_PIN_LED     22

#define ILI9488_PANEL_W     320
#define ILI9488_PANEL_H     480

#define ILI9488_DEFAULT_ROTATION 1
