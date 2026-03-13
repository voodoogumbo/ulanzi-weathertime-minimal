/*
  display.h - LED matrix display utilities (XY mapping, pixel ops, brightness)
*/

#ifndef DISPLAY_H
#define DISPLAY_H

#include <FastLED.h>
#include "hw_config.h"

// Shared display state
extern CRGB leds[];
extern uint8_t currentBrightness;
extern volatile bool displayDirty;

// TC001 serpentine matrix wiring - coordinate mapping
inline uint16_t XY(uint8_t x, uint8_t y) {
  if (y & 0x01) {
    // odd rows go right->left
    return (y * MW) + (MW - 1 - x);
  } else {
    // even rows go left->right
    return (y * MW) + x;
  }
}

// Set pixel with bounds checking
inline void pset(uint8_t x, uint8_t y, const CRGB& c) {
  if (x < MW && y < MH) leds[XY(x,y)] = c;
}

// Clear entire display
void clearAll();

// Time-based brightness adjustment (checked every 60s)
void updateBrightness();

#endif // DISPLAY_H
