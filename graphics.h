/*
  graphics.h - Weather icons, Christmas tree, BYU logo, boot icon
*/

#ifndef GRAPHICS_H
#define GRAPHICS_H

#include <FastLED.h>

// Draw weather icon with sophisticated gradients
void drawWeatherIcon(uint8_t iconIndex, int x, int y, const CRGB& c);

// Draw skull boot icon (displayed on watchdog restart)
void drawWatchdogBootIcon();

// Draw Christmas tree icon (8x8)
void drawChristmasTree(int x, int y);

// Draw BYU logo (16x8 blue oval with white Y)
void drawBYULogo(int x, int y);

// Draw LSU tiger eye logo (16x8 purple eyelid, gold iris, dark slit pupil)
void drawLSULogo(int x, int y);

// Draw arrow icon (8x8): iconIndex 0=down, 1=up
void drawArrowIcon(uint8_t iconIndex, int x, int y, const CRGB& c);

#endif // GRAPHICS_H
