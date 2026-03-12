// firmware/main/display.h
#ifndef DISPLAY_H
#define DISPLAY_H

#include "lvgl.h"

// Initialize SPI bus, ST7789 panel, LVGL display, and tick timer.
// Returns the LVGL display object. Starts backlight.
lv_display_t *display_init(void);

#endif // DISPLAY_H
