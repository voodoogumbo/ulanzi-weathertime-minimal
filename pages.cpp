/*
  pages.cpp - Page rendering implementations
*/

#include "pages.h"
#include "display.h"
#include "font_render.h"
#include "graphics.h"
#include "time_helpers.h"
#include "hw_config.h"

// Externs for page state
extern volatile bool notifyActive;
extern char notifyText[];
extern CRGB notifyColor;
extern unsigned long notifyEndMs;
extern int notifyScrollOffset;
extern unsigned long lastScrollMs;
extern uint16_t scrollSpeedMs;
extern int notifyTextWidth;
extern volatile bool notifyScrolling;
extern uint8_t notifyScrollLoops;
extern uint8_t notifyScrollMaxLoops;
extern volatile bool notifyAlternateColors;
extern volatile bool notifyBYUMode;
extern volatile bool notifyLSUMode;
extern volatile uint8_t notifyIconType;
extern char weatherCondition[];
extern char weatherIcon[];
extern float temperature;
extern volatile bool colonVisible;
extern PageType currentPage;
extern volatile bool christmasMode;

void drawClock() {
  struct tm timeinfo;
  if (!getCurrentTime(&timeinfo)) {
    clearAll();
    drawLargeString("TIME?", 2, 0, COLOR_DAY);
    return;
  }

  CRGB col;
  if (christmasMode) {
    col = CHRISTMAS_GREEN;
  } else if (isRecycleDay(timeinfo)) {
    col = COLOR_RECYCLE;
  } else if (isNight(timeinfo)) {
    col = COLOR_NIGHT;
  } else {
    col = COLOR_DAY;
  }

  clearAll();

  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;

  int x = 1;
  int y = 0;

  if (christmasMode) {
    drawLargeGlyph(hour / 10, x, y, CHRISTMAS_GREEN);
    x += DIGIT_W + GAP;
    drawLargeGlyph(hour % 10, x, y, CHRISTMAS_RED);
    x += DIGIT_W + GAP;
    if (colonVisible) {
      drawBigColon(x, y, CHRISTMAS_GOLD);
    }
    x += COLON_W + GAP;
    drawLargeGlyph(minute / 10, x, y, CHRISTMAS_GREEN);
    x += DIGIT_W + GAP;
    drawLargeGlyph(minute % 10, x, y, CHRISTMAS_RED);
  } else {
    drawLargeGlyph(hour / 10, x, y, col);
    x += DIGIT_W + GAP;
    drawLargeGlyph(hour % 10, x, y, col);
    x += DIGIT_W + GAP;
    if (colonVisible) {
      drawBigColon(x, y, col);
    }
    x += COLON_W + GAP;
    drawLargeGlyph(minute / 10, x, y, col);
    x += DIGIT_W + GAP;
    drawLargeGlyph(minute % 10, x, y, col);
  }
}

void drawWeather() {
  struct tm timeinfo;
  if (!getCurrentTime(&timeinfo)) {
    clearAll();
    drawLargeString("TIME?", 2, 0, COLOR_DAY);
    return;
  }

  CRGB col = isNight(timeinfo) ? COLOR_NIGHT : COLOR_DAY;

  clearAll();

  int temp = (int)round(temperature);
  CRGB tempColor = tempToColorF(temperature, isNight(timeinfo), currentBrightness);

  int x = 1;
  int y = 0;

  bool negative = (temp < 0);
  int absTemp = negative ? -temp : temp;

  if (negative) {
    for (int mx = 0; mx < 4; mx++) {
      pset(x + mx, y + 3, tempColor);
    }
    x += 5;
  }

  if (absTemp >= 100) {
    drawLargeGlyph(absTemp / 100, x, y, tempColor);
    x += DIGIT_SPACING;
    drawLargeGlyph((absTemp / 10) % 10, x, y, tempColor);
    x += DIGIT_SPACING;
    drawLargeGlyph(absTemp % 10, x, y, tempColor);
    x += 6;
  } else if (absTemp >= 10) {
    drawLargeGlyph(absTemp / 10, x, y, tempColor);
    x += DIGIT_SPACING;
    drawLargeGlyph(absTemp % 10, x, y, tempColor);
    x += 6;
  } else {
    drawLargeGlyph(absTemp, x, y, tempColor);
    x += 6;
  }

  drawLargeDegree(x - 1, y, tempColor);

  if (christmasMode) {
    drawChristmasTree(24, 0);
  } else {
    uint8_t iconIndex = 1;
    CRGB iconColor = CRGB(180, 180, 180);

    size_t iconLen = strlen(weatherIcon);
    bool isNightWeather = (iconLen > 0) && (weatherIcon[iconLen - 1] == 'n');

    if (strcmp(weatherCondition, "Clear") == 0) {
      if (isNightWeather) {
        iconIndex = 5;
        iconColor = CRGB(255, 200, 0);
      } else {
        iconIndex = 0;
        iconColor = CRGB(255, 200, 0);
      }
    }
    else if (strcmp(weatherCondition, "Clouds") == 0) {
      iconIndex = 1;
      iconColor = CRGB(180, 180, 180);
    }
    else if (strcmp(weatherCondition, "Rain") == 0 || strcmp(weatherCondition, "Drizzle") == 0) {
      iconIndex = 2;
      iconColor = CRGB(0, 120, 255);
    }
    else if (strcmp(weatherCondition, "Snow") == 0) {
      iconIndex = 3;
      iconColor = CRGB(255, 255, 255);
    }
    else if (strcmp(weatherCondition, "Thunderstorm") == 0) {
      iconIndex = 4;
      iconColor = CRGB(150, 0, 255);
    }

    drawWeatherIcon(iconIndex, 24, 0, iconColor);
  }
}

void drawNotify() {
  clearAll();

  if (notifyScrolling) {
    int y = 0;
    unsigned long now = millis();
    if (now - lastScrollMs >= scrollSpeedMs) {
      lastScrollMs = now;
      notifyScrollOffset--;

      if (notifyScrollOffset < -notifyTextWidth) {
        notifyScrollLoops++;
        if (notifyScrollLoops >= notifyScrollMaxLoops) {
          notifyActive = false;
          return;
        }
        notifyScrollOffset = MW;
      }
    }
    if (notifyBYUMode) {
      drawBYULogo(notifyScrollOffset, 0);
      drawLargeString(notifyText, notifyScrollOffset + 18, y, notifyColor);
    } else if (notifyLSUMode) {
      drawLSULogo(notifyScrollOffset, 0);
      drawLargeString(notifyText, notifyScrollOffset + 18, y, notifyColor);
    } else if (notifyIconType > 0) {
      drawArrowIcon(notifyIconType - 1, notifyScrollOffset, 0, notifyColor);
      drawLargeString(notifyText, notifyScrollOffset + 10, y, notifyColor);
    } else if (notifyAlternateColors) {
      drawLargeStringAlternating(notifyText, notifyScrollOffset, y, CHRISTMAS_GREEN, CHRISTMAS_RED);
    } else {
      drawLargeString(notifyText, notifyScrollOffset, y, notifyColor);
    }
  } else {
    int largeWidth = getLargeStringWidth(notifyText);
    if (largeWidth <= MW) {
      int x = (MW - largeWidth) / 2;
      drawLargeString(notifyText, x, 0, notifyColor);
    } else {
      int y = (MH - 5) / 2;
      int x = (MW - notifyTextWidth) / 2;
      if (x < 0) x = 0;
      drawSmallString(notifyText, x, y, notifyColor);
    }
  }
}
