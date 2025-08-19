/*
  TC001_Enhanced_SingleFile.ino — Complete Enhanced Clock in Single File
  
  🔧 SETUP INSTRUCTIONS:
    1. Copy config.h.example to config.h
    2. Fill in your WiFi credentials, MQTT broker, and OpenWeather API key
    3. Upload to your TC001 device
  
  ⭐ Features:
    - 32x8 LED matrix with AWTRIX TMODE5-style clock display
    - Improved colon separators (two separate 2x2 blocks)
    - Full 32-pixel width time display with optimized spacing
    - Enhanced temperature display with custom color scheme
    - Page rotation: Clock → Calendar → Weather
    - Night mode: after 22:00 local, brightness = 8 and text turns red
    - Smart weather icons with day/night variants (moon for clear nights)
    - MQTT listener with comprehensive command support
    - OpenWeather API integration with custom colored 8x8 weather icons
    - Mountain Time Zone with bulletproof DST support
    - Dual-core FreeRTOS architecture with robust watchdog management
    - Enhanced error handling and network stability
  
  📡 MQTT Topics (replace 'tc001' with your MQTT_BASE):
    - tc001/notify        - Show notification
    - tc001/page          - Change display page  
    - tc001/brightness    - Manual brightness control
    - tc001/auto_brightness - Toggle auto-brightness
    - tc001/config        - Runtime configuration
    - tc001/weather       - Force weather update
  
  🔐 SECURITY NOTE:
    Never commit config.h to public repositories! It contains sensitive credentials.
    Always use config.h.example as your template and keep config.h private.
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <time.h>
#include <esp_task_wdt.h>
#include "config.h"  // User configuration (WiFi, MQTT, OpenWeather API)

/*********** 
  USER CONFIGURATION: 
  All user settings (WiFi, MQTT, OpenWeather API) are now in config.h
  Copy config.h.example to config.h and customize for your setup.
***********/

/*********** HARDWARE CONFIGURATION ***********/
// LED Matrix Configuration
static const uint8_t MW = 32;              // Matrix width
static const uint8_t MH = 8;               // Matrix height
static const uint16_t NUM_LEDS = MW * MH;  // Total LEDs
#define LED_PIN  32                         // WS2812B data pin (GPIO32)
#define LED_TYPE WS2812B                    // LED chip type
#define COLOR_ORDER GRB                     // Color channel order

// Ambient Sensor Configuration
#define AMBIENT_SENSOR_PIN 35               // Light sensor (GPIO35)
static const uint16_t AMBIENT_MIN = 0;      // Minimum sensor reading (very dark)
static const uint16_t AMBIENT_MAX = 4095;   // Maximum sensor reading (very bright)
static const uint8_t BRIGHTNESS_MIN = 1;    // Minimum LED brightness
static const uint8_t BRIGHTNESS_MAX = 100;  // Maximum LED brightness

// Color Themes
static CRGB COLOR_DAY = CRGB(255, 255, 255);   // White for day mode
static CRGB COLOR_NIGHT = CRGB(255, 0, 0);     // Red for night mode

// Page System
enum PageType { PAGE_CLOCK, PAGE_CALENDAR, PAGE_WEATHER };

// Timing Configuration
static const unsigned long PAGE_DURATION_MS = 10000;        // 10 seconds per page
static const unsigned long WEATHER_UPDATE_INTERVAL_MS = 900000; // 15 minutes
static const unsigned long AMBIENT_READ_INTERVAL_MS = 1000;     // 1 second

// Clock Display Layout (AWTRIX TMODE5 style)
static const int DIGIT_W = 6;      // Digit width in pixels
static const int COLON_W = 2;      // Colon width in pixels  
static const int GAP = 1;          // Spacing between elements

// Temperature Display Layout
static const int DIGIT_SPACING = 8;  // 6px glyph + 2px gap for readability

/*********** FONT DATA STRUCTURES ***********/
struct LargeGlyph { uint8_t rows[7]; uint8_t w; };
struct SmallGlyph { uint8_t rows[5]; uint8_t w; };
struct SmallDigit { uint8_t rows[5]; uint8_t w; };

// Large digits for clock display (0-9)
PROGMEM static const LargeGlyph LARGE_DIGITS[10] = {
  // 0
  {{0b011110,0b100001,0b100001,0b100001,0b100001,0b100001,0b011110},6},
  // 1  
  {{0b001000,0b011000,0b001000,0b001000,0b001000,0b001000,0b111110},6},
  // 2
  {{0b011110,0b100001,0b000001,0b000010,0b001100,0b110000,0b111111},6},
  // 3
  {{0b011110,0b100001,0b000001,0b001110,0b000001,0b100001,0b011110},6},
  // 4
  {{0b000110,0b001010,0b010010,0b100010,0b111111,0b000010,0b000010},6},
  // 5
  {{0b111111,0b100000,0b111110,0b000001,0b000001,0b100001,0b011110},6},
  // 6
  {{0b011110,0b100000,0b100000,0b111110,0b100001,0b100001,0b011110},6},
  // 7
  {{0b111111,0b000001,0b000010,0b000100,0b001000,0b010000,0b100000},6},
  // 8
  {{0b011110,0b100001,0b100001,0b011110,0b100001,0b100001,0b011110},6},
  // 9
  {{0b011110,0b100001,0b100001,0b011111,0b000001,0b000001,0b011110},6}
};

// Small font for text (A-Z)
PROGMEM static const SmallGlyph SMALL_FONT[26] = {
  {{0b111,0b101,0b111,0b101,0b101},3}, // A
  {{0b110,0b101,0b110,0b101,0b110},3}, // B
  {{0b111,0b100,0b100,0b100,0b111},3}, // C
  {{0b110,0b101,0b101,0b101,0b110},3}, // D
  {{0b111,0b100,0b110,0b100,0b111},3}, // E
  {{0b111,0b100,0b110,0b100,0b100},3}, // F
  {{0b111,0b100,0b100,0b101,0b111},3}, // G
  {{0b101,0b101,0b111,0b101,0b101},3}, // H
  {{0b111,0b010,0b010,0b010,0b111},3}, // I
  {{0b111,0b001,0b001,0b101,0b111},3}, // J
  {{0b101,0b110,0b100,0b110,0b101},3}, // K
  {{0b100,0b100,0b100,0b100,0b111},3}, // L
  {{0b101,0b111,0b111,0b101,0b101},3}, // M
  {{0b101,0b111,0b111,0b111,0b101},3}, // N
  {{0b111,0b101,0b101,0b101,0b111},3}, // O
  {{0b111,0b101,0b111,0b100,0b100},3}, // P
  {{0b111,0b101,0b101,0b111,0b001},3}, // Q
  {{0b111,0b101,0b110,0b101,0b101},3}, // R
  {{0b111,0b100,0b111,0b001,0b111},3}, // S
  {{0b111,0b010,0b010,0b010,0b010},3}, // T
  {{0b101,0b101,0b101,0b101,0b111},3}, // U
  {{0b101,0b101,0b101,0b101,0b010},3}, // V
  {{0b101,0b101,0b111,0b111,0b101},3}, // W
  {{0b101,0b101,0b010,0b101,0b101},3}, // X
  {{0b101,0b101,0b111,0b010,0b010},3}, // Y
  {{0b111,0b001,0b010,0b100,0b111},3}  // Z
};

// Small digits for temperature display (0-9)
PROGMEM static const SmallDigit SMALL_DIGITS[10] = {
  {{0b111,0b101,0b101,0b101,0b111},3}, // 0
  {{0b010,0b110,0b010,0b010,0b111},3}, // 1
  {{0b111,0b001,0b111,0b100,0b111},3}, // 2
  {{0b111,0b001,0b111,0b001,0b111},3}, // 3
  {{0b101,0b101,0b111,0b001,0b001},3}, // 4
  {{0b111,0b100,0b111,0b001,0b111},3}, // 5
  {{0b111,0b100,0b111,0b101,0b111},3}, // 6
  {{0b111,0b001,0b010,0b100,0b100},3}, // 7
  {{0b111,0b101,0b111,0b101,0b111},3}, // 8
  {{0b111,0b101,0b111,0b001,0b111},3}  // 9
};

// Weather icons for different conditions
PROGMEM static const uint8_t WEATHER_ICONS[][8] = {
  // Clear sky (sun)
  {0b00011000,0b01111110,0b11111111,0b11111111,0b11111111,0b11111111,0b01111110,0b00011000},
  // Clouds  
  {0b00000000,0b00111100,0b01111110,0b11111111,0b11111111,0b01111110,0b00000000,0b00000000},
  // Rain
  {0b00111100,0b01111110,0b11111111,0b01111110,0b01010101,0b10101010,0b01010101,0b10101010},
  // Snow
  {0b00111100,0b01111110,0b11111111,0b01111110,0b10101010,0b01010101,0b10101010,0b01010101},
  // Thunderstorm
  {0b00111100,0b01111110,0b11111111,0b01111110,0b00010000,0b00110000,0b01100000,0b11000000},
  // Clear night (crescent moon)
  {0b00011000,0b00111000,0b01111000,0b01111000,0b01111000,0b01111000,0b00111000,0b00011000}
};

// Month abbreviations for calendar display
const char* const MONTH_NAMES[12] PROGMEM = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

/*********** GLOBAL STATE ***********/
// Network clients
WiFiClient espClient;
PubSubClient mqtt(espClient);
HTTPClient http;

// LED array
CRGB leds[NUM_LEDS];

// Page system state
PageType currentPage = PAGE_CLOCK;
bool rotationEnabled = true;
unsigned long pageDurationMs = PAGE_DURATION_MS;

// Notification state
bool notifyActive = false;
String notifyText;
CRGB notifyColor = CRGB(255, 255, 0);
unsigned long notifyEndMs = 0;

// Weather data
String weatherCondition = "";
String weatherIcon = "";
float temperature = 0.0;
unsigned long weatherUpdateIntervalMs = WEATHER_UPDATE_INTERVAL_MS;

// Animation state
bool colonVisible = true;

// Brightness control
bool autoBrightnessEnabled = true;
uint8_t manualBrightness = 24;
uint8_t currentBrightness = 24;
unsigned long lastAmbientReadMs = 0;

// FreeRTOS task handles
TaskHandle_t renderTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;
SemaphoreHandle_t displayMutex = NULL;

// Watchdog state
bool wdt_added = false;

/*********** DISPLAY UTILITIES ***********/
// TC001 uses serpentine matrix wiring - coordinate mapping
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
void clearAll() { 
  fill_solid(leds, NUM_LEDS, CRGB::Black); 
}

// Check if current time is night mode (22:00-06:00)
bool isNight(const struct tm& timeinfo) {
  return (timeinfo.tm_hour >= 22 || timeinfo.tm_hour < 6);
}

/*********** FONT RENDERING ***********/
// Draw large glyph with bold effect (AWTRIX-style) - horizontal dilation
void drawLargeGlyphBold(uint8_t digitIndex, int x, int y, const CRGB& c) {
  if (digitIndex >= 10) return; // bounds check
  
  // Safely read PROGMEM data
  LargeGlyph g;
  memcpy_P(&g, &LARGE_DIGITS[digitIndex], sizeof(LargeGlyph));
  
  for (uint8_t ry = 0; ry < 7; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      bool on = row & (1 << (5 - rx));
      bool left_on = (rx > 0) && (row & (1 << (5 - (rx - 1))));
      if (on || left_on) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

// Draw large glyph (6x7 font) - safely read from PROGMEM
void drawLargeGlyph(uint8_t digitIndex, int x, int y, const CRGB& c) {
  if (digitIndex >= 10) return; // bounds check
  
  // Safely read PROGMEM data
  LargeGlyph g;
  memcpy_P(&g, &LARGE_DIGITS[digitIndex], sizeof(LargeGlyph));
  
  for (uint8_t ry = 0; ry < 7; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      if (row & (1 << (5 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

// Draw small glyph (3x5 font) - safely read from PROGMEM
void drawSmallGlyph(uint8_t charIndex, int x, int y, const CRGB& c) {
  if (charIndex >= 26) return; // bounds check
  
  // Safely read PROGMEM data
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

// Draw small digit (3x5 font) - safely read from PROGMEM
void drawSmallDigit(uint8_t digit, int x, int y, const CRGB& c) {
  if (digit > 9) return; // bounds check
  
  // Safely read PROGMEM data
  SmallDigit g;
  memcpy_P(&g, &SMALL_DIGITS[digit], sizeof(SmallDigit));
  
  for (uint8_t ry = 0; ry < 5; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      if (row & (1 << (2 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

// Draw string with small font - supports A-Z, 0-9
void drawSmallString(const String& str, int x, int y, const CRGB& c) {
  int currentX = x;
  for (int i = 0; i < str.length(); i++) {
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
    } else if (ch == ' ') {
      currentX += 2;
    } else {
      currentX += 2; // unknown char spacing
    }
  }
}

// Draw degree symbol (3x3 ring) - top-aligned for large temps
void drawLargeDegree(int x, int y, const CRGB& c) {
  // Larger degree symbol for better visibility with large temp numbers
  pset(x+1, y-3, c);  // moved up 3 pixels
  pset(x+0, y-2, c);  pset(x+2, y-2, c);  
  pset(x+1, y-1, c);
}

// Draw big colon (AWTRIX-style) - two separate 2x2 blocks
void drawBigColon(int x, int y, const CRGB& c) {
  // Top 2x2 block
  pset(x,   y+1, c); pset(x+1, y+1, c);
  pset(x,   y+2, c); pset(x+1, y+2, c);
  // Gap between blocks for traditional colon appearance
  // Bottom 2x2 block  
  pset(x,   y+4, c); pset(x+1, y+4, c);
  pset(x,   y+5, c); pset(x+1, y+5, c);
}

// Draw weather icon (8x8 pixels)
void drawWeatherIcon(uint8_t iconIndex, int x, int y, const CRGB& c) {
  if (iconIndex >= 6) return; // bounds check
  
  for (uint8_t ry = 0; ry < 8; ++ry) {
    uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
    for (uint8_t rx = 0; rx < 8; ++rx) {
      if (row & (1 << (7 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

/*********** BRIGHTNESS CONTROL ***********/
// Read ambient sensor and map to brightness value
uint8_t readAmbientBrightness() {
  uint16_t ambientReading = analogRead(AMBIENT_SENSOR_PIN);
  
  // Apply smoothing (simple moving average with previous reading)
  static uint16_t prevReading = 2048; // Initialize to mid-range
  ambientReading = (ambientReading + prevReading) / 2;
  prevReading = ambientReading;
  
  // Map ambient reading to brightness range with some curve adjustment
  uint8_t brightness;
  if (ambientReading < 500) {
    brightness = BRIGHTNESS_MIN;
  } else if (ambientReading > 3500) {
    brightness = BRIGHTNESS_MAX;
  } else {
    brightness = map(ambientReading, 500, 3500, BRIGHTNESS_MIN + 5, BRIGHTNESS_MAX - 10);
  }
  
  return constrain(brightness, BRIGHTNESS_MIN, BRIGHTNESS_MAX);
}

// Update display brightness based on ambient sensor and time
void updateBrightness() {
  if (autoBrightnessEnabled) {
    currentBrightness = readAmbientBrightness();
  } else {
    currentBrightness = manualBrightness;
  }
  
  // Apply night mode override if enabled
  struct tm timeinfo;
  if (getLocalTime(&timeinfo)) {
    if (isNight(timeinfo)) {
      static bool night_mode_logged = false;
      if (!night_mode_logged) {
        Serial.printf("Night mode active (%02d:%02d), brightness forced to 8\n", timeinfo.tm_hour, timeinfo.tm_min);
        night_mode_logged = true;
      }
      currentBrightness = 8; // Override with night mode brightness for visibility
    } else {
      static bool night_mode_logged = false;
      night_mode_logged = false;
    }
  }
}

/*********** PAGE RENDERING ***********/
// Draw main clock page with full-width AWTRIX TMODE5 style
void drawClock() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    // Time not synced yet, show visible sync message
    clearAll();
    CRGB col = COLOR_DAY;
    FastLED.setBrightness(currentBrightness);
    drawSmallString("SYNC", 10, 2, col);
    return;
  }

  // Timezone debugging - log every minute to track time accuracy
  static int last_logged_minute = -1;
  if (timeinfo.tm_min != last_logged_minute) {
    time_t now = time(nullptr);
    struct tm utc_tm;
    gmtime_r(&now, &utc_tm);
    Serial.printf("Time Debug - UTC: %02d:%02d, Local: %02d:%02d, DST: %s\n",
                  utc_tm.tm_hour, utc_tm.tm_min,
                  timeinfo.tm_hour, timeinfo.tm_min,
                  timeinfo.tm_isdst ? "Yes" : "No");
    last_logged_minute = timeinfo.tm_min;
  }

  CRGB col = isNight(timeinfo) ? COLOR_NIGHT : COLOR_DAY;
  FastLED.setBrightness(currentBrightness);

  clearAll();

  // AWTRIX TMODE5-style layout: bold digits, full 32-pixel width
  int hour = timeinfo.tm_hour;
  int minute = timeinfo.tm_min;

  // Perfect 32-pixel width: 6×4 digits + 2×2 gaps + 2×1 colon = 32 pixels
  int x = 0;  // start at left edge for full width
  int y = 0;  // start at top

  // Draw HH with bold digits
  drawLargeGlyphBold(hour / 10, x, y, col);     
  x += DIGIT_W + GAP;
  drawLargeGlyphBold(hour % 10, x, y, col);     
  x += DIGIT_W + GAP;

  // Draw big colon (blinking) - 2x2 dot blocks
  if (colonVisible) {
    drawBigColon(x, y, col);
  }
  x += COLON_W + GAP;

  // Draw MM with bold digits
  drawLargeGlyphBold(minute / 10, x, y, col);   
  x += DIGIT_W + GAP;
  drawLargeGlyphBold(minute % 10, x, y, col);
}

// Draw calendar page with month and date
void drawCalendar() {
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    return;
  }

  CRGB col = isNight(timeinfo) ? COLOR_NIGHT : COLOR_DAY;
  FastLED.setBrightness(currentBrightness);

  clearAll();

  // Get month name
  char monthName[4];
  strcpy_P(monthName, (char*)pgm_read_ptr(&(MONTH_NAMES[timeinfo.tm_mon])));
  
  // Draw month on left side, vertically centered
  drawSmallString(monthName, 1, 2, col);

  // Draw date on right side
  int day = timeinfo.tm_mday;

  if (day >= 10) {
    // Two digits: position on right side of 32-pixel display
    int x = 19;  // Start position for two 6-wide digits + spacing
    drawLargeGlyph(day / 10, x, 1, col);
    drawLargeGlyph(day % 10, x + 7, 1, col);
  } else {
    // Single digit: position on right side
    int x = 22; // Center single 6-wide digit on right side
    drawLargeGlyph(day, x, 1, col);
  }
}

// Draw weather page with temperature and icon
void drawWeather() {
  CRGB col = COLOR_DAY; // Weather page always uses day colors
  FastLED.setBrightness(currentBrightness);

  clearAll();

  // Draw temperature with improved spacing for better readability
  int temp = (int)round(temperature);
  
  // Use large 6x7 digits for temperature with better spacing
  int x = 1;
  int y = 1;
  
  // Draw temperature digits with improved spacing
  if (temp >= 100) {
    // Three digits (100+) - rare but handle gracefully
    drawLargeGlyphBold(temp / 100, x, y, col);
    x += DIGIT_SPACING;
    drawLargeGlyphBold((temp / 10) % 10, x, y, col);
    x += DIGIT_SPACING;
    drawLargeGlyphBold(temp % 10, x, y, col);
    x += 6;  // Final digit width only
  } else if (temp >= 10) {
    // Two digits (10-99) - most common case
    drawLargeGlyphBold(temp / 10, x, y, col);
    x += DIGIT_SPACING;
    drawLargeGlyphBold(temp % 10, x, y, col);
    x += 6;  // Final digit width only
  } else {
    // Single digit (0-9)
    drawLargeGlyphBold(temp, x, y, col);
    x += 6;  // Final digit width only
  }
  
  // Draw degree symbol with better positioning (top-aligned)
  drawLargeDegree(x + 1, y, col);  // Start at y position, +1px spacing from digits
  
  // Draw weather icon on right side with custom color scheme
  uint8_t iconIndex = 1; // Default to clouds
  CRGB iconColor = CRGB(180, 180, 180); // Default to light gray (clouds)
  
  // Use OpenWeather API icon codes to detect day/night variants
  bool isNightWeather = weatherIcon.endsWith("n");
  
  if (weatherCondition == "Clear") {
    if (isNightWeather) {
      iconIndex = 5;  // Moon icon for clear night
      iconColor = CRGB(200, 220, 255); // Silver-blue for moon
    } else {
      iconIndex = 0;  // Sun icon for clear day
      iconColor = CRGB(255, 200, 0); // Golden yellow for sun
    }
  }
  else if (weatherCondition == "Clouds") {
    iconIndex = 1;  // Clouds icon
    iconColor = CRGB(180, 180, 180); // Light gray
  }
  else if (weatherCondition == "Rain" || weatherCondition == "Drizzle") {
    iconIndex = 2;  // Rain icon
    iconColor = CRGB(0, 120, 255); // Deep blue
  }
  else if (weatherCondition == "Snow") {
    iconIndex = 3;  // Snow icon
    iconColor = CRGB(255, 255, 255); // Pure white
  }
  else if (weatherCondition == "Thunderstorm") {
    iconIndex = 4;  // Thunderstorm icon
    iconColor = CRGB(150, 0, 255); // Electric purple
  }
  
  drawWeatherIcon(iconIndex, 24, 0, iconColor);  // Position on right side
}

// Draw notification overlay
void drawNotify() {
  clearAll();
  // Simple notification display (reuse small font)
  drawSmallString(notifyText, 2, 2, notifyColor);
}

/*********** TIME MANAGEMENT ***********/
// Setup NTP time synchronization with bulletproof timezone handling
void setupTime() {
  Serial.printf("Contacting NTP servers: pool.ntp.org, time.nist.gov");
  unsigned long startTime = millis();
  
  // Use ESP32 native timezone configuration with configTzTime
  const char* TZ_DENVER = "MST7MDT,M3.2.0/2,M11.1.0/2";  // Mountain Time with DST rules
  configTzTime(TZ_DENVER, "pool.ntp.org", "time.nist.gov");
  
  // Wait up to 10 seconds for time sync with watchdog resets
  struct tm timeinfo;
  while (!getLocalTime(&timeinfo) && (millis() - startTime) < 10000) {
    if (wdt_added) esp_task_wdt_reset();  // Feed watchdog during NTP sync
    delay(500);
    Serial.print(".");
  }
  
  if (!getLocalTime(&timeinfo)) {
    Serial.printf(" ❌ NTP sync failed after %lu ms! Clock will show 'SYNC' until sync completes.\n", millis() - startTime);
    Serial.println("Note: NTP will continue trying in background.");
  } else {
    // Get UTC time for comparison
    time_t now = time(nullptr);
    struct tm utc_tm;
    gmtime_r(&now, &utc_tm);
    
    const char* dst_status = timeinfo.tm_isdst > 0 ? "MDT (UTC-6)" : timeinfo.tm_isdst == 0 ? "MST (UTC-7)" : "Unknown";
    Serial.printf(" ✅ NTP sync success! (%lu ms)\n", millis() - startTime);
    Serial.printf("    UTC Time: %02d:%02d:%02d\n", utc_tm.tm_hour, utc_tm.tm_min, utc_tm.tm_sec);
    Serial.printf("    Local Time: %02d:%02d:%02d %s\n", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec, dst_status);
    Serial.printf("    DST Active: %s\n", timeinfo.tm_isdst ? "Yes" : "No");
  }
}

/*********** WIFI MANAGEMENT ***********/
// Ensure WiFi connection with shorter timeout for dual-core stability
void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  Serial.print("WiFi connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  // Shorter timeout for dual-core stability (5 seconds instead of 10)
  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    // Feed watchdog during WiFi connection attempts
    if (wdt_added) esp_task_wdt_reset();
    delay(250);  // Shorter delay intervals
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" Failed! Will retry next cycle.");
    return;
  }

  Serial.printf(" WiFi connected! IP: %s, RSSI: %d dBm\n", 
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

/*********** WEATHER API ***********/
// Fetch weather data from OpenWeather API with enhanced timeouts
void fetchWeatherData() {
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Weather fetch skipped - WiFi not connected");
    return;
  }

  Serial.println("Fetching weather data...");
  char url[256];
  snprintf(url, 256, "http://api.openweathermap.org/data/2.5/weather?lat=%s&lon=%s&appid=%s&units=imperial", 
           OPENWEATHER_LAT, OPENWEATHER_LON, OPENWEATHER_API_KEY);

  http.begin(url);
  http.setTimeout(3000);       // 3 second timeout (shorter for dual-core stability)
  http.setConnectTimeout(2000); // 2 second connection timeout
  http.setReuse(false);        // Disable reuse to prevent connection hangs
  if (wdt_added) esp_task_wdt_reset();   // Feed WDT before potentially long operation
  int httpCode = http.GET();
  if (wdt_added) esp_task_wdt_reset();   // Feed WDT after potentially long operation
  Serial.printf("Weather API HTTP response: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    String payload = http.getString();
    Serial.printf("Weather API response length: %d bytes\n", payload.length());
    
    StaticJsonDocument<1024> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (!error) {
      temperature = doc["main"]["temp"];
      weatherCondition = doc["weather"][0]["main"].as<String>();
      weatherIcon = doc["weather"][0]["icon"].as<String>();
      Serial.printf("Weather updated: %.1fF, %s, icon=%s\n", temperature, weatherCondition.c_str(), weatherIcon.c_str());
    } else {
      Serial.printf("Weather JSON parse error: %s\n", error.c_str());
    }
  } else {
    Serial.printf("Weather API request failed: HTTP %d\n", httpCode);
  }
  http.end();
}

/*********** MQTT COMMAND HANDLERS ***********/
// Handle notification display command
void handleNotify(const JsonDocument& doc) {
  notifyText = doc["text"].as<const char*>();
  const char* hex = doc["color"] | "FFFF00";
  unsigned long v = strtoul(hex, nullptr, 16);
  notifyColor = CRGB((v>>16)&0xFF, (v>>8)&0xFF, v&0xFF);
  uint16_t dur = doc["duration"] | 4;
  notifyEndMs = millis() + dur*1000UL;
  notifyActive = true;
}

// Handle page change command
void handlePageCommand(const JsonDocument& doc) {
  String page = doc["page"].as<String>();
  if (page == "clock") currentPage = PAGE_CLOCK;
  else if (page == "calendar") currentPage = PAGE_CALENDAR;
  else if (page == "weather") currentPage = PAGE_WEATHER;
  // Page timer is now handled locally by render task
}

// Handle manual brightness setting
void handleBrightness(const JsonDocument& doc) {
  uint8_t brightness = doc["brightness"] | 24;
  brightness = constrain(brightness, 1, 255);
  manualBrightness = brightness;
  autoBrightnessEnabled = false; // Manual override disables auto-brightness
}

// Handle auto-brightness configuration
void handleAutoBrightness(const JsonDocument& doc) {
  if (doc.containsKey("enabled")) {
    autoBrightnessEnabled = doc["enabled"];
  }
}

// Handle general configuration settings
void handleConfig(const JsonDocument& doc) {
  if (doc.containsKey("page_duration")) {
    pageDurationMs = (doc["page_duration"].as<uint16_t>()) * 1000UL;
  }
  if (doc.containsKey("rotation_enabled")) {
    rotationEnabled = doc["rotation_enabled"];
  }
  if (doc.containsKey("weather_update_minutes")) {
    weatherUpdateIntervalMs = (doc["weather_update_minutes"].as<uint16_t>()) * 60 * 1000UL;
  }
}

/*********** MQTT MANAGEMENT ***********/
// Ensure MQTT connection and subscribe to all topics
void ensureMqtt() {
  if (mqtt.connected()) return;
  
  // Feed watchdog before potentially long MQTT operations
  if (wdt_added) esp_task_wdt_reset();
  
  char cid[24];
  snprintf(cid, 24, "tc001-%llX", ESP.getEfuseMac());
  Serial.printf("Attempting MQTT connection to %s:%d with client ID: %s\n", MQTT_HOST, MQTT_PORT, cid);

  // Add timeout for MQTT connection attempts to prevent indefinite blocking
  unsigned long connectStart = millis();
  if (mqtt.connect(cid)) {
    Serial.println("🎉 MQTT CONNECTED SUCCESSFULLY!");
    
    char topic[128];
    bool allSubscribed = true;
    
    // Feed watchdog during subscription process
    if (wdt_added) esp_task_wdt_reset();
    
    snprintf(topic, 128, "%s/notify", MQTT_BASE);
    if (mqtt.subscribe(topic)) {
      Serial.printf("✅ Subscribed to: %s\n", topic);
    } else {
      Serial.printf("❌ Failed to subscribe to: %s\n", topic);
      allSubscribed = false;
    }
    
    snprintf(topic, 128, "%s/page", MQTT_BASE);
    if (mqtt.subscribe(topic)) {
      Serial.printf("✅ Subscribed to: %s\n", topic);
    } else {
      Serial.printf("❌ Failed to subscribe to: %s\n", topic);
      allSubscribed = false;
    }
    
    snprintf(topic, 128, "%s/brightness", MQTT_BASE);
    if (mqtt.subscribe(topic)) {
      Serial.printf("✅ Subscribed to: %s\n", topic);
    } else {
      Serial.printf("❌ Failed to subscribe to: %s\n", topic);
      allSubscribed = false;
    }
    
    // Feed watchdog mid-way through subscriptions
    if (wdt_added) esp_task_wdt_reset();
    
    snprintf(topic, 128, "%s/auto_brightness", MQTT_BASE);
    if (mqtt.subscribe(topic)) {
      Serial.printf("✅ Subscribed to: %s\n", topic);
    } else {
      Serial.printf("❌ Failed to subscribe to: %s\n", topic);
      allSubscribed = false;
    }
    
    snprintf(topic, 128, "%s/config", MQTT_BASE);
    if (mqtt.subscribe(topic)) {
      Serial.printf("✅ Subscribed to: %s\n", topic);
    } else {
      Serial.printf("❌ Failed to subscribe to: %s\n", topic);
      allSubscribed = false;
    }
    
    snprintf(topic, 128, "%s/weather", MQTT_BASE);
    if (mqtt.subscribe(topic)) {
      Serial.printf("✅ Subscribed to: %s\n", topic);
    } else {
      Serial.printf("❌ Failed to subscribe to: %s\n", topic);
      allSubscribed = false;
    }
    
    if (allSubscribed) {
      Serial.println("🔥 ALL MQTT SUBSCRIPTIONS SUCCESSFUL - Ready for messages!");
    } else {
      Serial.println("⚠️  Some MQTT subscriptions failed - check broker permissions");
    }
    
  } else {
    Serial.printf("❌ MQTT CONNECTION FAILED - Error code: %d (attempt took %lu ms)\n", mqtt.state(), millis() - connectStart);
    Serial.println("MQTT Error Codes: -4=timeout, -3=lost, -2=failed, -1=disconnected, 0=bad protocol, 1=bad client ID, 2=unavailable, 3=bad credentials, 4=unauthorized");
    
    // Add small delay to prevent rapid retry attempts that could block watchdog
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

// MQTT message callback - routes to appropriate handlers
void mqttCallback(char* topic, byte* payload, unsigned int len) {
  char json[len + 1];
  memcpy(json, payload, len);
  json[len] = '\0';

  Serial.printf("✅ MQTT CALLBACK TRIGGERED - Topic: '%s', Length: %d\n", topic, len);
  Serial.printf("📧 Raw Payload: %s\n", json);
  
  // Debug: Print payload as hex to see if there are hidden characters
  Serial.print("🔍 Hex Dump: ");
  for (unsigned int i = 0; i < len; i++) {
    Serial.printf("%02X ", payload[i]);
  }
  Serial.println();

  StaticJsonDocument<512> doc;
  DeserializationError err = deserializeJson(doc, json);
  if (err) {
    Serial.printf("❌ MQTT JSON parse error: %s\n", err.c_str());
    Serial.printf("📝 Attempted to parse: '%s'\n", json);
    
    // Try to fix common JSON issues
    String fixedJson = String(json);
    fixedJson.replace("{text:", "{\"text\":");
    fixedJson.replace(",color:", ",\"color\":");
    fixedJson.replace(",duration:", ",\"duration\":");
    fixedJson.replace(",page:", ",\"page\":");
    fixedJson.replace(",brightness:", ",\"brightness\":");
    fixedJson.replace(",enabled:", ",\"enabled\":");
    
    Serial.printf("🔧 Attempting to fix JSON: '%s'\n", fixedJson.c_str());
    err = deserializeJson(doc, fixedJson);
    if (err) {
      Serial.printf("❌ Still failed after fix attempt: %s\n", err.c_str());
      return;
    } else {
      Serial.println("✅ JSON fixed and parsed successfully!");
    }
  } else {
    Serial.println("✅ JSON parsed successfully on first try!");
  }

  // Build topic strings for comparison
  char baseTopic[strlen(MQTT_BASE) + 1];
  strcpy(baseTopic, MQTT_BASE);

  char notifyTopic[strlen(baseTopic) + 8];
  strcpy(notifyTopic, baseTopic);
  strcat(notifyTopic, "/notify");

  char pageTopic[strlen(baseTopic) + 6];
  strcpy(pageTopic, baseTopic);
  strcat(pageTopic, "/page");

  char brightnessTopic[strlen(baseTopic) + 12];
  strcpy(brightnessTopic, baseTopic);
  strcat(brightnessTopic, "/brightness");

  char autoBrightnessTopic[strlen(baseTopic) + 17];
  strcpy(autoBrightnessTopic, baseTopic);
  strcat(autoBrightnessTopic, "/auto_brightness");

  char configTopic[strlen(baseTopic) + 8];
  strcpy(configTopic, baseTopic);
  strcat(configTopic, "/config");

  char weatherTopic[strlen(baseTopic) + 9];
  strcpy(weatherTopic, baseTopic);
  strcat(weatherTopic, "/weather");

  // Route to appropriate handlers
  if (strcmp(topic, notifyTopic) == 0) {
    handleNotify(doc);
  } else if (strcmp(topic, pageTopic) == 0) {
    handlePageCommand(doc);
  } else if (strcmp(topic, brightnessTopic) == 0) {
    handleBrightness(doc);
  } else if (strcmp(topic, autoBrightnessTopic) == 0) {
    handleAutoBrightness(doc);
  } else if (strcmp(topic, configTopic) == 0) {
    handleConfig(doc);
  } else if (strcmp(topic, weatherTopic) == 0) {
    fetchWeatherData();
  }
}

/*********** FREERTOS TASKS ***********/
// Render Task - Core 1: High-priority display and UI updates
void renderTask(void *pvParameters) {
  unsigned long lastRenderMs = 0;
  unsigned long lastBlinkMs = 0;
  unsigned long lastPageChangeMs = 0;
  
  Serial.println("Render task started on Core 1");
  
  while (true) {
    unsigned long currentMs = millis();
    
    // Feed watchdog for render task
    if (wdt_added) esp_task_wdt_reset();
    
    // Handle colon blinking for clock (500ms intervals)
    if (currentMs - lastBlinkMs > 500) {
      lastBlinkMs = currentMs;
      colonVisible = !colonVisible;
    }
    
    // Handle page rotation (non-blocking)
    if (rotationEnabled && !notifyActive && (currentMs - lastPageChangeMs > pageDurationMs)) {
      lastPageChangeMs = currentMs;
      currentPage = (PageType)((currentPage + 1) % 3);
    }
    
    // Render at 30 FPS (33ms intervals) for smooth display
    if (currentMs - lastRenderMs >= 33) {
      lastRenderMs = currentMs;
      
      // Take display mutex for thread-safe rendering
      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10))) {
        // Handle ambient sensor reading (only if auto-brightness enabled)
        if (autoBrightnessEnabled && (currentMs - lastAmbientReadMs > AMBIENT_READ_INTERVAL_MS)) {
          lastAmbientReadMs = currentMs;
          updateBrightness();
        }
        
        // Draw current page or notification
        if (notifyActive) {
          drawNotify();
          if ((long)(currentMs - notifyEndMs) >= 0) {
            notifyActive = false;
          }
        } else {
          switch (currentPage) {
            case PAGE_CLOCK:
              drawClock();
              break;
            case PAGE_CALENDAR:
              drawCalendar();
              break;
            case PAGE_WEATHER:
              drawWeather();
              break;
          }
        }
        
        FastLED.show();
        xSemaphoreGive(displayMutex);
      }
    }
    
    // Yield to prevent task starvation (essential for dual-core stability)
    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

// Network Task - Core 0: Background network operations
void networkTask(void *pvParameters) {
  unsigned long lastWeatherCheck = 0;
  
  Serial.println("Network task started on Core 0");
  
  while (true) {
    // Feed watchdog for network task at start of loop
    if (wdt_added) esp_task_wdt_reset();
    
    // Handle network operations with timeouts and watchdog feeds
    ensureWifi();
    
    // Feed watchdog after WiFi operations
    if (wdt_added) esp_task_wdt_reset();
    
    ensureMqtt();
    
    // Feed watchdog after MQTT connection operations
    if (wdt_added) esp_task_wdt_reset();
    
    mqtt.loop();
    
    // Feed watchdog after MQTT loop processing
    if (wdt_added) esp_task_wdt_reset();
    
    unsigned long currentMs = millis();
    
    // Handle weather updates (non-blocking with timeout)
    if (currentMs - lastWeatherCheck > weatherUpdateIntervalMs) {
      lastWeatherCheck = currentMs;
      fetchWeatherData();
      
      // Feed watchdog after weather API call
      if (wdt_added) esp_task_wdt_reset();
    }
    
    // Network task runs every 100ms to balance responsiveness and CPU usage
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

/*********** ARDUINO SETUP ***********/
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TC001 Enhanced Clock Starting (Single File Version) ===");

  // Initialize LED matrix
  Serial.println("Initializing LED matrix...");
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  
  // Startup test: briefly show bright display to verify screen works
  Serial.println("Testing display with bright startup pattern...");
  FastLED.setBrightness(50);  // Temporarily use higher brightness for test
  fill_solid(leds, NUM_LEDS, CRGB(255, 255, 255));  // Fill with white
  FastLED.show();
  delay(2000);  // Show for 2 seconds (safe - WDT not armed yet)
  
  // Set normal brightness
  updateBrightness();
  FastLED.setBrightness(currentBrightness);
  clearAll();
  FastLED.show();
  Serial.printf("LED matrix initialized with brightness: %d\n", currentBrightness);

  // Initialize network
  ensureWifi();

  // Allow network stack to fully initialize
  Serial.println("Allowing network to stabilize...");
  for (int i = 0; i < 10; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Network ready!");

  // Setup time synchronization
  Serial.println("Setting up time synchronization...");
  setupTime();

  // Configure MQTT
  Serial.printf("Configuring MQTT for server %s:%d\n", MQTT_HOST, MQTT_PORT);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);  // Increase buffer for larger JSON payloads
  mqtt.setKeepAlive(20);    // 20 second keepalive for stability
  mqtt.setSocketTimeout(3); // 3 second socket timeout to prevent hangs
  mqtt.setCallback(mqttCallback);

  // Initial weather fetch
  Serial.println("Fetching initial weather data...");
  fetchWeatherData();
  
  // Create display mutex for thread-safe operations
  displayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL) {
    Serial.println("Failed to create display mutex!");
    while (1) delay(1000);
  }
  
  // Configure watchdog timer - only watch our tasks, not idle cores
  if (!wdt_added) {
    Serial.println("Configuring watchdog timer for dual-core tasks...");
    esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 15000,  // 15 second timeout (increased for network operations)
      .idle_core_mask = 0,  // Don't watch idle cores (prevents false triggers)
      .trigger_panic = true
    };
    esp_task_wdt_init(&twdt_config);
    wdt_added = true;
    Serial.println("Watchdog timer configured for tasks only.");
  }
  
  // Create FreeRTOS tasks for dual-core operation
  Serial.println("Creating FreeRTOS tasks...");
  
  // Render task on Core 1 (higher priority for smooth display)
  xTaskCreatePinnedToCore(
    renderTask,           // Task function
    "RenderTask",         // Task name
    4096,                 // Stack size
    NULL,                 // Task parameters
    2,                    // Priority (higher than network)
    &renderTaskHandle,    // Task handle
    1                     // Core 1 (dedicated to rendering)
  );
  
  // Network task on Core 0 (lower priority for background operations)
  xTaskCreatePinnedToCore(
    networkTask,          // Task function
    "NetworkTask",        // Task name
    8192,                 // Stack size (larger for network operations)
    NULL,                 // Task parameters
    1,                    // Priority (lower than render)
    &networkTaskHandle,   // Task handle
    0                     // Core 0 (WiFi/MQTT operations)
  );
  
  // Add tasks to watchdog monitoring
  if (renderTaskHandle != NULL) {
    esp_task_wdt_add(renderTaskHandle);
    Serial.println("✅ Render task created and added to watchdog (Core 1)");
  }
  if (networkTaskHandle != NULL) {
    esp_task_wdt_add(networkTaskHandle);
    Serial.println("✅ Network task created and added to watchdog (Core 0)");
  }
  
  Serial.println("=== Dual-core setup complete ===");
}

/*********** ARDUINO LOOP ***********/
void loop() {
  // In dual-core mode, the main loop just monitors system health
  // All rendering and network operations are handled by dedicated tasks
  
  // Monitor task health and provide emergency fallback
  if (renderTaskHandle != NULL && eTaskGetState(renderTaskHandle) == eDeleted) {
    Serial.println("⚠️ Render task died! Attempting restart...");
    xTaskCreatePinnedToCore(renderTask, "RenderTask", 4096, NULL, 2, &renderTaskHandle, 1);
    if (renderTaskHandle != NULL) {
      esp_task_wdt_add(renderTaskHandle);
    }
  }
  
  if (networkTaskHandle != NULL && eTaskGetState(networkTaskHandle) == eDeleted) {
    Serial.println("⚠️ Network task died! Attempting restart...");
    xTaskCreatePinnedToCore(networkTask, "NetworkTask", 8192, NULL, 1, &networkTaskHandle, 0);
    if (networkTaskHandle != NULL) {
      esp_task_wdt_add(networkTaskHandle);
    }
  }
  
  // Main loop runs every 5 seconds for system monitoring
  vTaskDelay(pdMS_TO_TICKS(5000));
}