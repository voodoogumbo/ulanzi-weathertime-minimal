/*
  time_helpers.cpp - Time utility implementations
*/

#include "time_helpers.h"
#include "hw_config.h"

// Spinlock for atomic snapshot of the time triple (mqttUnixTime, mqttTimeMs, mqttTimeAvailable)
portMUX_TYPE timeMux = portMUX_INITIALIZER_UNLOCKED;

bool getCurrentTime(struct tm* timeinfo) {
  // Snapshot time triple atomically to prevent torn reads across cores
  bool available;
  time_t unixTime;
  unsigned long timeMs;

  taskENTER_CRITICAL(&timeMux);
  available = mqttTimeAvailable;
  unixTime = mqttUnixTime;
  timeMs = mqttTimeMs;
  taskEXIT_CRITICAL(&timeMux);

  if (!available || (millis() - timeMs) >= 1800000) return false;
  time_t currentTime = unixTime + ((millis() - timeMs) / 1000);
  localtime_r(&currentTime, timeinfo);
  return true;
}

bool isNight(const struct tm& timeinfo) {
  return (timeinfo.tm_hour >= 22 || timeinfo.tm_hour < 6);
}

bool isRecycleDay(const struct tm& timeinfo) {
  static bool cachedResult = false;
  static int lastCheckedHour = -1;
  static int lastCheckedDay = -1;

  if (timeinfo.tm_hour == lastCheckedHour && timeinfo.tm_mday == lastCheckedDay) {
    return cachedResult;
  }

  struct tm startDate = {0};
  startDate.tm_year = RECYCLE_START_YEAR - 1900;
  startDate.tm_mon = RECYCLE_START_MONTH - 1;
  startDate.tm_mday = RECYCLE_START_DAY;
  startDate.tm_isdst = -1;

  time_t startTime = mktime(&startDate);
  struct tm currentDateOnly = timeinfo;
  currentDateOnly.tm_hour = 0;
  currentDateOnly.tm_min = 0;
  currentDateOnly.tm_sec = 0;
  time_t currentTime = mktime(&currentDateOnly);

  int daysDiff = (currentTime - startTime) / 86400;
  int dayOfWeek = timeinfo.tm_wday;

  bool isMondayRecycleDay = (dayOfWeek == 1) && (daysDiff % RECYCLE_INTERVAL_DAYS == 0);
  bool isSundayBeforeRecycle = (dayOfWeek == 0) && ((daysDiff + 1) % RECYCLE_INTERVAL_DAYS == 0);

  bool newResult = isMondayRecycleDay || isSundayBeforeRecycle;
  if (newResult != cachedResult) {
    Serial.printf("Recycle status: %s\n", newResult ? "YES" : "NO");
  }
  cachedResult = newResult;
  lastCheckedHour = timeinfo.tm_hour;
  lastCheckedDay = timeinfo.tm_mday;

  return cachedResult;
}

bool isChristmasSeason(const struct tm& timeinfo) {
  static bool cachedResult = false;
  static int lastCheckedHour = -1;
  static int lastCheckedDay = -1;

  if (timeinfo.tm_hour == lastCheckedHour && timeinfo.tm_mday == lastCheckedDay) {
    return cachedResult;
  }

  bool newResult = (timeinfo.tm_mon == 11 &&
                    timeinfo.tm_mday >= CHRISTMAS_START_DAY &&
                    timeinfo.tm_mday <= CHRISTMAS_END_DAY);

  if (newResult != cachedResult) {
    Serial.printf("Christmas mode: %s\n", newResult ? "ACTIVE" : "off");
  }
  cachedResult = newResult;
  lastCheckedHour = timeinfo.tm_hour;
  lastCheckedDay = timeinfo.tm_mday;
  return cachedResult;
}

// Helper: 0..1 -> 0..255 fract8 for FastLED blending
static inline uint8_t u01_to_fract8(float u) {
  if (u < 0) u = 0;
  if (u > 1) u = 1;
  return (uint8_t)(u * 255.0f + 0.5f);
}

// Boost cold colors to counteract perceived dimness on WS2812
static inline CRGB liftCold(const CRGB& c) {
  uint8_t r = qadd8(c.r, 12);
  uint8_t g = qadd8(c.g, 12);
  uint8_t b = qadd8(c.b, 12);
  return CRGB(r, g, b);
}

// Brightness compensation - boosts color vibrancy when brightness is low
static CRGB compensateForBrightness(CRGB color, uint8_t brightness) {
  if (brightness >= 60) {
    return color;
  }

  float brightnessFactor = brightness / 100.0f;
  float compensationAmount = (1.0f - brightnessFactor) * 0.4f;

  uint8_t r = qadd8(color.r, (uint8_t)(compensationAmount * 255));
  uint8_t g = qadd8(color.g, (uint8_t)(compensationAmount * 255));
  uint8_t b = qadd8(color.b, (uint8_t)(compensationAmount * 255));

  return CRGB(r, g, b);
}

CRGB tempToColorF(float tF, bool nightMode, uint8_t brightness) {
  if (nightMode) {
    return COLOR_NIGHT;
  }

  CRGB baseColor;

  if (tF <= 0.0f) {
    float u = (tF - T_MIN_F) / (0.0f - T_MIN_F);
    baseColor = blend(ICE, WHITE, u01_to_fract8(u));
    baseColor = liftCold(baseColor);
  } else {
    float u = (tF - 0.0f) / (T_MAX_F - 0.0f);
    baseColor = blend(WHITE, HOT, u01_to_fract8(u));
  }

  return compensateForBrightness(baseColor, brightness);
}
