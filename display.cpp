/*
  display.cpp - LED matrix display utility implementations
*/

#include "display.h"
#include "time_helpers.h"

void clearAll() {
  fill_solid(leds, NUM_LEDS, CRGB::Black);
}

void updateBrightness() {
  static unsigned long lastUpdateMs = 0;
  if (millis() - lastUpdateMs < 60000 && lastUpdateMs != 0) return;
  lastUpdateMs = millis();

  struct tm timeinfo;
  if (!getCurrentTime(&timeinfo)) return;

  int minuteOfDay = timeinfo.tm_hour * 60 + timeinfo.tm_min;
  uint8_t target = (minuteOfDay >= 390 && minuteOfDay < 1320)
                   ? BRIGHTNESS_DAY : BRIGHTNESS_NIGHT;
  if (target != currentBrightness) {
    currentBrightness = target;
    FastLED.setBrightness(currentBrightness);
    displayDirty = true;
  }
}
