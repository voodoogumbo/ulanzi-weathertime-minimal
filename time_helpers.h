/*
  time_helpers.h - Time utility functions (night mode, recycle day, Christmas, temp colors)
*/

#ifndef TIME_HELPERS_H
#define TIME_HELPERS_H

#include <time.h>
#include <FastLED.h>

// Spinlock for atomic read/write of the time triple across cores
extern portMUX_TYPE timeMux;

// Time source state
extern volatile bool mqttTimeAvailable;
extern volatile time_t mqttUnixTime;
extern volatile unsigned long mqttTimeMs;

// Get current time via MQTT time source
bool getCurrentTime(struct tm* timeinfo);

// Check if current time is night mode (22:00-06:00)
bool isNight(const struct tm& timeinfo);

// Check if current day is a recycling day
bool isRecycleDay(const struct tm& timeinfo);

// Check if current date is in Christmas season (Dec 20-26)
bool isChristmasSeason(const struct tm& timeinfo);

// Temperature to color mapping with night mode and brightness compensation
CRGB tempToColorF(float tF, bool nightMode, uint8_t brightness);

#endif // TIME_HELPERS_H
