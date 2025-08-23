/*
  TC001 Enhanced Clock - Complete single-file firmware
  Features: AWTRIX-style display, MQTT time sync, weather integration, 
  cloud gradients, dual-core FreeRTOS, Mountain Time DST support
  
  Setup: Copy config.h.example → config.h, configure, upload
  MQTT Topics: notify, page, brightness, auto_brightness, config, weather, time
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <time.h>
#include <esp_task_wdt.h>
#include "config.h"  // User configuration (WiFi, MQTT, OpenWeather API)

// Hardware Configuration
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

// Font Data
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

// Weather icons for different conditions + Watchdog boot icon
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
  {0b00011000,0b00111000,0b01111000,0b01111000,0b01111000,0b01111000,0b00111000,0b00011000},
  // Watchdog icon (index 6) - LaMetric style dog face
  {0b01100110,0b11111111,0b11011011,0b11111111,0b11111111,0b01111110,0b01011010,0b00111100}
};

// Month abbreviations for calendar display
const char* const MONTH_NAMES[12] PROGMEM = {
  "JAN", "FEB", "MAR", "APR", "MAY", "JUN",
  "JUL", "AUG", "SEP", "OCT", "NOV", "DEC"
};

// Global State
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

// MQTT Time Management
bool mqttTimeAvailable = false;
time_t mqttUnixTime = 0;
unsigned long mqttTimeMs = 0;
bool useMqttTime = true;

// FreeRTOS task handles
TaskHandle_t renderTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;
SemaphoreHandle_t displayMutex = NULL;

// Watchdog state
bool wdt_added = false;

// Display Utilities
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

// Font Rendering
// Draw large glyph (6x7 font) with optional bold effect
void drawLargeGlyph(uint8_t digitIndex, int x, int y, const CRGB& c, bool bold = false) {
  if (digitIndex >= 10) return;
  
  LargeGlyph g;
  memcpy_P(&g, &LARGE_DIGITS[digitIndex], sizeof(LargeGlyph));
  
  for (uint8_t ry = 0; ry < 7; ++ry) {
    uint8_t row = g.rows[ry];
    for (uint8_t rx = 0; rx < g.w; ++rx) {
      bool on = row & (1 << (5 - rx));
      bool left_on = bold && (rx > 0) && (row & (1 << (5 - (rx - 1))));
      if (on || left_on) {
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

// Draw weather icon (8x8 pixels) with sophisticated cloud gradient
void drawWeatherIcon(uint8_t iconIndex, int x, int y, const CRGB& c) {
  if (iconIndex >= 7) return; // bounds check
  
  // Special sophisticated gradient for cloudy weather (index 1)
  if (iconIndex == 1) {
    // Define gradient colors for natural cloud depth
    CRGB darkGray = CRGB(60, 60, 60);        // Bottom shadow
    CRGB lighterGray = CRGB(120, 120, 120);  // Mid-tone
    CRGB evenLighterGray = CRGB(200, 200, 200); // Top highlight
    
    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          CRGB pixelColor;
          
          // Apply sophisticated 5-row gradient (cloud spans rows 1-5)
          if (ry == 1) {
            // Row 1 (top): Top highlight - brightest
            pixelColor = evenLighterGray;
          } else if (ry == 2) {
            // Row 2: Light gray + rightmost pixel highlight
            if (rx == 7) {
              pixelColor = evenLighterGray; // Right edge highlight
            } else {
              pixelColor = CRGB(180, 180, 180); // Light gray
            }
          } else if (ry == 3) {
            // Row 3: Light gray + rightmost pixel transition
            if (rx == 7) {
              pixelColor = lighterGray; // Right edge transition
            } else {
              pixelColor = CRGB(180, 180, 180); // Light gray
            }
          } else if (ry == 4) {
            // Row 4: Medium gray + rightmost pixel shadow
            if (rx == 7) {
              pixelColor = darkGray; // Right edge shadow
            } else {
              pixelColor = lighterGray;
            }
          } else if (ry == 5) {
            // Row 5 (bottom): Dark gray shadow base
            pixelColor = darkGray;
          } else {
            // Any other rows (0, 6, 7): Use original color if present
            pixelColor = c;
          }
          
          pset(x + rx, y + ry, pixelColor);
        }
      }
    }
  } else {
    // Draw other icons normally
    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          pset(x + rx, y + ry, c);
        }
      }
    }
  }
}

// Draw watchdog boot icon (LaMetric style)
void drawWatchdogBootIcon() {
  clearAll();
  
  // Display watchdog icon (index 6) in LaMetric orange
  CRGB laMetricOrange = CRGB(255, 140, 0); // LaMetric signature orange
  drawWeatherIcon(6, 12, 0, laMetricOrange); // Centered on 32x8 display
  
  FastLED.setBrightness(80); // Bright for boot message
  FastLED.show();
  
  // Display for 2 seconds
  delay(2000);
  
  // Feed watchdog during display
  if (wdt_added) esp_task_wdt_reset();
}

// Brightness Control
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

// Update display brightness based on ambient sensor only
void updateBrightness() {
  if (autoBrightnessEnabled) {
    currentBrightness = readAmbientBrightness();
  } else {
    currentBrightness = manualBrightness;
  }
  // Note: Time-based night mode affects text COLOR only, not LED brightness
  // Night mode color logic is handled in individual display functions
}

/*********** PAGE RENDERING ***********/
// Get current time - MQTT time only (NTP removed for performance)
bool getCurrentTime(struct tm* timeinfo, bool* usingMqttTime) {
  *usingMqttTime = false;
  
  // Check if MQTT time is available and recent (within last 5 minutes)
  if (useMqttTime && mqttTimeAvailable && 
      (millis() - mqttTimeMs) < 300000) {  // 5 minutes = 300000ms
    
    // Calculate current time based on MQTT timestamp + elapsed time
    time_t currentTime = mqttUnixTime + ((millis() - mqttTimeMs) / 1000);
    
    // Convert to local time (timezone already set during setup)
    localtime_r(&currentTime, timeinfo);
    
    // Debug time calculation every minute
    static int lastDebugMinute = -1;
    if (timeinfo->tm_min != lastDebugMinute) {
      struct tm utc_tm;
      gmtime_r(&currentTime, &utc_tm);
      Serial.printf("Time Debug - UTC: %02d:%02d, Local: %02d:%02d, Night Mode: %s\n",
                    utc_tm.tm_hour, utc_tm.tm_min,
                    timeinfo->tm_hour, timeinfo->tm_min,
                    isNight(*timeinfo) ? "YES" : "NO");
      lastDebugMinute = timeinfo->tm_min;
    }
    
    *usingMqttTime = true;
    return true;
  }
  
  // No time available - MQTT time required
  return false;
}

// Draw main clock page with full-width AWTRIX TMODE5 style
void drawClock() {
  struct tm timeinfo;
  bool usingMqttTime;
  if (!getCurrentTime(&timeinfo, &usingMqttTime)) {
    // No MQTT time available, show error message
    clearAll();
    CRGB col = COLOR_DAY;
    FastLED.setBrightness(currentBrightness);
    drawSmallString("NO TIME", 6, 2, col);
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
  drawLargeGlyph(hour / 10, x, y, col, true);     
  x += DIGIT_W + GAP;
  drawLargeGlyph(hour % 10, x, y, col, true);     
  x += DIGIT_W + GAP;

  // Draw big colon (blinking) - 2x2 dot blocks
  if (colonVisible) {
    drawBigColon(x, y, col);
  }
  x += COLON_W + GAP;

  // Draw MM with bold digits
  drawLargeGlyph(minute / 10, x, y, col, true);   
  x += DIGIT_W + GAP;
  drawLargeGlyph(minute % 10, x, y, col, true);
  
  // Add subtle time source indicator in bottom right corner
  if (usingMqttTime) {
    pset(31, 7, CRGB(0, 255, 0)); // Green pixel for MQTT time
  } else {
    pset(31, 7, CRGB(0, 100, 255)); // Blue pixel for NTP time
  }
}

// Draw calendar page with month and date
void drawCalendar() {
  struct tm timeinfo;
  bool usingMqttTime;
  if (!getCurrentTime(&timeinfo, &usingMqttTime)) {
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
  struct tm timeinfo;
  bool usingMqttTime;
  if (!getCurrentTime(&timeinfo, &usingMqttTime)) {
    // No MQTT time available, show error message
    clearAll();
    CRGB col = COLOR_DAY;
    FastLED.setBrightness(currentBrightness);
    drawSmallString("NO TIME", 6, 2, col);
    return;
  }

  CRGB col = isNight(timeinfo) ? COLOR_NIGHT : COLOR_DAY;
  FastLED.setBrightness(currentBrightness);

  clearAll();

  // Draw temperature with color scaling from white (0°F) to red (100°F+)
  int temp = (int)round(temperature);
  
  // Calculate temperature color: white at 0°F, red at 100°F+
  CRGB tempColor;
  
  // Override with night mode color during 22:00-06:00
  if (isNight(timeinfo)) {
    tempColor = COLOR_NIGHT; // Force red during night hours
    static bool nightModeLogged = false;
    if (!nightModeLogged) {
      Serial.printf("NIGHT MODE TRIGGERED: Temperature forced to red at %02d:%02d\n", 
                    timeinfo.tm_hour, timeinfo.tm_min);
      nightModeLogged = true;
    }
  } else {
    static bool nightModeLogged = false;
    nightModeLogged = false; // Reset for next night
    if (temp <= 0) {
      tempColor = CRGB(255, 255, 255); // White for 0 and below
    } else if (temp >= 100) {
      tempColor = CRGB(255, 0, 0); // Pure red for 100+ degrees
    } else {
      // Smooth transition from white to red based on temperature
      // At 0°F: (255, 255, 255) -> At 100°F: (255, 0, 0)
      uint8_t red = 255;
      uint8_t green = map(temp, 0, 100, 255, 0); // Green decreases as temp increases
      uint8_t blue = map(temp, 0, 100, 255, 0);  // Blue decreases as temp increases
      tempColor = CRGB(red, green, blue);
    }
  }
  
  // Use large 6x7 digits for temperature with better spacing
  int x = 1;
  int y = 1;
  
  // Draw temperature digits with color scaling
  if (temp >= 100) {
    // Three digits (100+) - rare but handle gracefully
    drawLargeGlyph(temp / 100, x, y, tempColor, true);
    x += DIGIT_SPACING;
    drawLargeGlyph((temp / 10) % 10, x, y, tempColor, true);
    x += DIGIT_SPACING;
    drawLargeGlyph(temp % 10, x, y, tempColor, true);
    x += 6;  // Final digit width only
  } else if (temp >= 10) {
    // Two digits (10-99) - most common case
    drawLargeGlyph(temp / 10, x, y, tempColor, true);
    x += DIGIT_SPACING;
    drawLargeGlyph(temp % 10, x, y, tempColor, true);
    x += 6;  // Final digit width only
  } else {
    // Single digit (0-9)
    drawLargeGlyph(temp, x, y, tempColor, true);
    x += 6;  // Final digit width only
  }
  
  // Draw degree symbol with same color as temperature
  drawLargeDegree(x + 1, y, tempColor);  // Start at y position, +1px spacing from digits
  
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


// WiFi Management
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

// Weather API
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

// MQTT Command Handlers
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

// Handle MQTT time updates with Unix timestamps
void handleTimeCommand(const JsonDocument& doc) {
  if (doc.containsKey("unix_time")) {
    mqttUnixTime = doc["unix_time"].as<time_t>();
    mqttTimeMs = millis();
    mqttTimeAvailable = true;
    Serial.printf("MQTT time received: %lu (Unix timestamp)\n", mqttUnixTime);
  }
}

// MQTT Management
// Helper function to subscribe to a topic
bool mqttSubscribe(const char* suffix, bool& allSubscribed) {
  char topic[128];
  snprintf(topic, 128, "%s%s", MQTT_BASE, suffix);
  if (mqtt.subscribe(topic)) {
    Serial.printf("✅ Subscribed to: %s\n", topic);
    return true;
  } else {
    Serial.printf("❌ Failed to subscribe to: %s\n", topic);
    allSubscribed = false;
    return false;
  }
}

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
    
    bool allSubscribed = true;
    
    // Subscribe to all topics using helper function
    if (wdt_added) esp_task_wdt_reset();
    mqttSubscribe("/notify", allSubscribed);
    mqttSubscribe("/page", allSubscribed);
    mqttSubscribe("/brightness", allSubscribed);
    mqttSubscribe("/auto_brightness", allSubscribed);
    if (wdt_added) esp_task_wdt_reset();
    mqttSubscribe("/config", allSubscribed);
    mqttSubscribe("/weather", allSubscribed);
    mqttSubscribe("/time", allSubscribed);
    
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
  // Feed watchdog at start of MQTT callback to prevent timeouts during processing
  if (wdt_added) esp_task_wdt_reset();
  
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
    Serial.printf("📝 Failed payload: '%s'\n", json);
    return; // Exit immediately on JSON parse error (no fixing attempts)
  }
  
  Serial.println("✅ JSON parsed successfully!");

  // Route to handlers using suffix matching (more efficient)
  String topicStr = String(topic);
  if (topicStr.endsWith("/notify")) {
    handleNotify(doc);
  } else if (topicStr.endsWith("/page")) {
    handlePageCommand(doc);
  } else if (topicStr.endsWith("/brightness")) {
    handleBrightness(doc);
  } else if (topicStr.endsWith("/auto_brightness")) {
    handleAutoBrightness(doc);
  } else if (topicStr.endsWith("/config")) {
    handleConfig(doc);
  } else if (topicStr.endsWith("/weather")) {
    fetchWeatherData();
  } else if (topicStr.endsWith("/time")) {
    handleTimeCommand(doc);
  }
}

// FreeRTOS Tasks
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
    
    // Handle page rotation (non-blocking) - only clock and weather pages
    if (rotationEnabled && !notifyActive && (currentMs - lastPageChangeMs > pageDurationMs)) {
      lastPageChangeMs = currentMs;
      currentPage = (currentPage == PAGE_CLOCK) ? PAGE_WEATHER : PAGE_CLOCK;
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

// Arduino Setup
void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TC001 Enhanced Clock Starting (Single File Version) ===");

  // Initialize LED matrix
  Serial.println("Initializing LED matrix...");
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  
  // Startup message: show LaMetric watchdog icon
  Serial.println("Showing watchdog boot icon...");
  drawWatchdogBootIcon();
  
  // Set normal brightness
  updateBrightness();
  FastLED.setBrightness(currentBrightness);
  clearAll();
  FastLED.show();
  Serial.printf("LED matrix initialized with brightness: %d\n", currentBrightness);

  // Setup timezone for MQTT time conversion (Mountain Time with DST)
  Serial.println("Setting up Mountain Time timezone...");
  const char* TZ_DENVER = "MST7MDT,M3.2.0/2,M11.1.0/2";
  setenv("TZ", TZ_DENVER, 1);
  tzset();
  Serial.println("Timezone configured: Mountain Time (MST/MDT with automatic DST)");

  // Initialize network
  ensureWifi();

  // Allow network stack to fully initialize
  Serial.println("Allowing network to stabilize...");
  for (int i = 0; i < 10; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Network ready!");

  // NTP removed for performance - time sync now via MQTT only
  Serial.println("Time sync via MQTT only (NTP disabled for performance)");
  // setupTime(); // Commented out - using MQTT time instead

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
      .timeout_ms = 30000,  // 30 second timeout (more lenient for stability)
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

// Arduino Loop
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