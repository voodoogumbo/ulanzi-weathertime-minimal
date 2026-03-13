/*
  font_render.cpp - Font rendering implementations
*/

#include "font_render.h"
#include "display.h"
#include "font_data.h"

void drawLargeGlyph(uint8_t digitIndex, int x, int y, const CRGB& c) {
  if (digitIndex >= 10) return;

  LargeGlyph g;
  memcpy_P(&g, &LARGE_DIGITS[digitIndex], sizeof(LargeGlyph));

  for (uint8_t ry = 0; ry < 8; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      if (row & (1 << (5 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

void drawLargeChar(char ch, int x, int y, const CRGB& c) {
  if (ch == ':') {
    pset(x, y + 2, c);
    pset(x, y + 4, c);
    return;
  }
  if (ch == '-') {
    pset(x, y + 3, c);
    pset(x + 1, y + 3, c);
    pset(x + 2, y + 3, c);
    return;
  }
  if (ch == '~' || ch == '^') {
    uint8_t idx = (ch == '~') ? 0 : 1;  // ~ = down, ^ = up
    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&ARROW_ICONS[idx][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          pset(x + rx, y + ry, c);
        }
      }
    }
    return;
  }

  LargeGlyph g;

  if (ch >= '0' && ch <= '9') {
    memcpy_P(&g, &LARGE_DIGITS[ch - '0'], sizeof(LargeGlyph));
  } else if (ch >= 'A' && ch <= 'Z') {
    memcpy_P(&g, &LARGE_LETTERS[ch - 'A'], sizeof(LargeGlyph));
  } else if (ch >= 'a' && ch <= 'z') {
    memcpy_P(&g, &LARGE_LETTERS[ch - 'a'], sizeof(LargeGlyph));
  } else {
    return;
  }

  for (uint8_t ry = 0; ry < 8; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      if (row & (1 << (g.w - 1 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

void drawLargeString(const char* str, int x, int y, const CRGB& c) {
  int currentX = x;
  for (int i = 0; str[i] != '\0'; i++) {
    char ch = str[i];
    if (ch >= '0' && ch <= '9') {
      drawLargeChar(ch, currentX, y, c);
      currentX += 7;
    } else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      drawLargeChar(ch, currentX, y, c);
      currentX += 6;
    } else if (ch == ':') {
      drawLargeChar(ch, currentX, y, c);
      currentX += 2;
    } else if (ch == '-') {
      drawLargeChar(ch, currentX, y, c);
      currentX += 4;
    } else if (ch == '~' || ch == '^') {
      drawLargeChar(ch, currentX, y, c);
      currentX += 9;
    } else if (ch == ' ') {
      currentX += 3;
    } else {
      currentX += 3;
    }
  }
}

void drawLargeStringAlternating(const char* str, int x, int y, const CRGB& c1, const CRGB& c2) {
  int currentX = x;
  for (int i = 0; str[i] != '\0'; i++) {
    char ch = str[i];
    CRGB color = (i % 2 == 0) ? c1 : c2;
    if (ch >= '0' && ch <= '9') {
      drawLargeChar(ch, currentX, y, color);
      currentX += 7;
    } else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      drawLargeChar(ch, currentX, y, color);
      currentX += 6;
    } else if (ch == ':') {
      drawLargeChar(ch, currentX, y, color);
      currentX += 2;
    } else if (ch == '-') {
      drawLargeChar(ch, currentX, y, color);
      currentX += 4;
    } else if (ch == '~' || ch == '^') {
      drawLargeChar(ch, currentX, y, color);
      currentX += 9;
    } else if (ch == ' ') {
      currentX += 3;
    } else {
      currentX += 3;
    }
  }
}

int getLargeStringWidth(const char* str) {
  int width = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    char ch = str[i];
    if (ch >= '0' && ch <= '9') {
      width += 7;
    } else if ((ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      width += 6;
    } else if (ch == ':') {
      width += 2;
    } else if (ch == '-') {
      width += 4;
    } else if (ch == '~' || ch == '^') {
      width += 9;
    } else if (ch == ' ') {
      width += 3;
    } else {
      width += 3;
    }
  }
  return width > 0 ? width - 1 : 0;
}

void drawSmallGlyph(uint8_t charIndex, int x, int y, const CRGB& c) {
  if (charIndex >= 26) return;

  SmallGlyph g;
  memcpy_P(&g, &SMALL_FONT[charIndex], sizeof(SmallGlyph));

  for (uint8_t ry = 0; ry < 5; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      if (row & (1 << (2 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

void drawSmallDigit(uint8_t digit, int x, int y, const CRGB& c) {
  if (digit > 9) return;

  SmallGlyph g;
  memcpy_P(&g, &SMALL_DIGITS[digit], sizeof(SmallGlyph));

  for (uint8_t ry = 0; ry < 5; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      if (row & (1 << (2 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

void drawSmallString(const char* str, int x, int y, const CRGB& c) {
  int currentX = x;
  for (int i = 0; str[i] != '\0'; i++) {
    uint8_t ch = str[i];

    if (ch >= '0' && ch <= '9') {
      drawSmallDigit(ch - '0', currentX, y, c);
      currentX += 4;
    } else if (ch >= 'A' && ch <= 'Z') {
      drawSmallGlyph(ch - 'A', currentX, y, c);
      currentX += 4;
    } else if (ch >= 'a' && ch <= 'z') {
      drawSmallGlyph(ch - 'a', currentX, y, c);
      currentX += 4;
    } else if (ch == ':') {
      pset(currentX, y + 1, c);
      pset(currentX, y + 3, c);
      currentX += 2;
    } else if (ch == '-') {
      pset(currentX, y + 2, c);
      pset(currentX + 1, y + 2, c);
      currentX += 3;
    } else if (ch == ' ') {
      currentX += 2;
    } else {
      currentX += 2;
    }
  }
}

int getSmallStringWidth(const char* str) {
  int width = 0;
  for (int i = 0; str[i] != '\0'; i++) {
    uint8_t ch = str[i];
    if ((ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'Z') || (ch >= 'a' && ch <= 'z')) {
      width += 4;
    } else if (ch == ' ') {
      width += 2;
    } else {
      width += 2;
    }
  }
  return width > 0 ? width - 1 : 0;
}

void drawLargeDegree(int x, int y, const CRGB& c) {
  (void)c;  // always white
  pset(x+2, y, CRGB(255, 255, 255));
}

void drawBigColon(int x, int y, const CRGB& c) {
  pset(x,   y+1, c); pset(x+1, y+1, c);
  pset(x,   y+2, c); pset(x+1, y+2, c);
  pset(x,   y+4, c); pset(x+1, y+4, c);
  pset(x,   y+5, c); pset(x+1, y+5, c);
}
