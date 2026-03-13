/*
  font_render.h - Font rendering functions for large and small glyphs
*/

#ifndef FONT_RENDER_H
#define FONT_RENDER_H

#include <FastLED.h>

// Large font (6x8 digits, 5x8 letters)
void drawLargeGlyph(uint8_t digitIndex, int x, int y, const CRGB& c);
void drawLargeChar(char ch, int x, int y, const CRGB& c);
void drawLargeString(const char* str, int x, int y, const CRGB& c);
void drawLargeStringAlternating(const char* str, int x, int y, const CRGB& c1, const CRGB& c2);
int getLargeStringWidth(const char* str);

// Small font (3x5)
void drawSmallGlyph(uint8_t charIndex, int x, int y, const CRGB& c);
void drawSmallDigit(uint8_t digit, int x, int y, const CRGB& c);
void drawSmallString(const char* str, int x, int y, const CRGB& c);
int getSmallStringWidth(const char* str);

// Special glyphs
void drawLargeDegree(int x, int y, const CRGB& c);
void drawBigColon(int x, int y, const CRGB& c);

#endif // FONT_RENDER_H
