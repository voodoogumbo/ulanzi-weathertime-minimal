/*
  font_data.h - PROGMEM glyph arrays and icon bitmaps
*/

#ifndef FONT_DATA_H
#define FONT_DATA_H

#include <Arduino.h>

// Font glyph structures
struct LargeGlyph { uint8_t rows[8]; uint8_t w; };
struct SmallGlyph { uint8_t rows[5]; uint8_t w; };

// Large 6x8 digit glyphs (0-9)
extern const LargeGlyph LARGE_DIGITS[10];

// Large 5x8 letter glyphs (A-Z)
extern const LargeGlyph LARGE_LETTERS[26];

// Small 3x5 letter glyphs (A-Z)
extern const SmallGlyph SMALL_FONT[26];

// Small 3x5 digit glyphs (0-9)
extern const SmallGlyph SMALL_DIGITS[10];

// Weather icons (8x8 bitmaps) + boot icon
extern const uint8_t WEATHER_ICONS[7][8];

// Arrow icons (8x8 bitmaps): 0=down, 1=up
extern const uint8_t ARROW_ICONS[2][8];

#endif // FONT_DATA_H
