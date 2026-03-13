/*
  hw_config.h - Hardware constants, pin definitions, enums, and timing
*/

#ifndef HW_CONFIG_H
#define HW_CONFIG_H

#include <FastLED.h>

// LED Matrix Configuration
static const uint8_t MW = 32;              // Matrix width
static const uint8_t MH = 8;               // Matrix height
static const uint16_t NUM_LEDS = MW * MH;  // Total LEDs
#define LED_PIN  32                         // WS2812B data pin (GPIO32)
#define LED_TYPE WS2812B                    // LED chip type
#define COLOR_ORDER GRB                     // Color channel order

// Buzzer Configuration
#define BUZZER_PIN 15

// Time-based Brightness
static const uint8_t BRIGHTNESS_DAY = 85;   // 6:30 AM - 10:00 PM
static const uint8_t BRIGHTNESS_NIGHT = 4;  // 10:00 PM - 6:30 AM

// Color Themes
static const CRGB COLOR_DAY = CRGB(255, 255, 255);     // White for day mode
static const CRGB COLOR_NIGHT = CRGB(255, 0, 0);       // Red for night mode
static const CRGB COLOR_RECYCLE = CRGB(0, 255, 0);     // Green for recycle indicator

// Recycling Schedule Configuration
static const int RECYCLE_START_YEAR = 2025;
static const int RECYCLE_START_MONTH = 11;      // November (1-12)
static const int RECYCLE_START_DAY = 17;        // 17th
static const int RECYCLE_INTERVAL_DAYS = 14;   // Every 14 days

// Christmas Mode Configuration
static const int CHRISTMAS_START_DAY = 20;   // December 20
static const int CHRISTMAS_END_DAY = 26;     // December 26
static const unsigned long CHRISTMAS_SCROLL_INTERVAL_MS = 600000; // 10 minutes

// Christmas Colors
static const CRGB CHRISTMAS_RED = CRGB(255, 0, 0);
static const CRGB CHRISTMAS_GREEN = CRGB(0, 255, 0);
static const CRGB CHRISTMAS_GOLD = CRGB(255, 200, 0);

// Temperature Color System - Diverging scale (ice-blue -> white -> red)
static const float T_MIN_F = -20.0f; // Cold floor
static const float T_MAX_F = 100.0f; // Hot ceiling

// Temperature color constants - bright enough for WS2812
static const CRGB ICE = CRGB(80, 180, 255);    // Ice blue that's readable
static const CRGB WHITE = CRGB(255, 255, 255);  // Pure white
static const CRGB HOT = CRGB(255, 0, 0);       // Pure red

// Page System
enum PageType { PAGE_CLOCK, PAGE_WEATHER };

// Timing Configuration
static const unsigned long PAGE_DURATION_MS = 10000;        // 10 seconds per page
static const unsigned long WEATHER_UPDATE_INTERVAL_MS = 900000; // 15 minutes

// Clock Display Layout (AWTRIX TMODE5 style)
static const int DIGIT_W = 6;      // Digit width in pixels
static const int COLON_W = 2;      // Colon width in pixels
static const int GAP = 1;          // Spacing between elements

// Temperature Display Layout
static const int DIGIT_SPACING = 7;  // 6px glyph + 1px gap

#endif // HW_CONFIG_H
