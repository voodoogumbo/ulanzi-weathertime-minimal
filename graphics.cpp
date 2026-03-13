/*
  graphics.cpp - Weather icon, Christmas tree, BYU logo, boot icon implementations
*/

#include "graphics.h"
#include "display.h"
#include "font_data.h"
#include "hw_config.h"
#include <esp_task_wdt.h>

// Module externs
extern uint8_t currentBrightness;
extern bool wdt_added;

void drawWeatherIcon(uint8_t iconIndex, int x, int y, const CRGB& c) {
  if (iconIndex >= 7) return;

  uint8_t boost = 0;
  if (currentBrightness < 15) {
    boost = map(currentBrightness, 1, 15, 120, 0);
  } else if (currentBrightness < 40) {
    boost = map(currentBrightness, 15, 40, 40, 0);
  }

  if (iconIndex == 1) {
    // Cloud gradient
    CRGB darkGray = CRGB(60 + boost, 60 + boost, 60 + boost);
    CRGB lighterGray = CRGB(qadd8(120, boost), qadd8(120, boost), qadd8(120, boost));
    CRGB evenLighterGray = CRGB(qadd8(200, boost), qadd8(200, boost), qadd8(200, boost));

    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          CRGB pixelColor;

          if (ry == 1) {
            pixelColor = evenLighterGray;
          } else if (ry == 2) {
            if (rx == 7) {
              pixelColor = evenLighterGray;
            } else {
              pixelColor = CRGB(qadd8(180, boost), qadd8(180, boost), qadd8(180, boost));
            }
          } else if (ry == 3) {
            if (rx == 7) {
              pixelColor = lighterGray;
            } else {
              pixelColor = CRGB(qadd8(180, boost), qadd8(180, boost), qadd8(180, boost));
            }
          } else if (ry == 4) {
            if (rx == 7) {
              pixelColor = darkGray;
            } else {
              pixelColor = lighterGray;
            }
          } else if (ry == 5) {
            pixelColor = darkGray;
          } else {
            pixelColor = c;
          }

          pset(x + rx, y + ry, pixelColor);
        }
      }
    }
  } else if (iconIndex == 2) {
    // Rain cloud gradient
    CRGB lightBlue = CRGB(qadd8(100, boost), qadd8(180, boost), 255);
    CRGB mediumBlue = CRGB(qadd8(60, boost), qadd8(140, boost), 255);
    CRGB deepBlue = CRGB(boost, qadd8(120, boost), 255);

    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          CRGB pixelColor;

          if (ry == 1) {
            pixelColor = lightBlue;
          } else if (ry == 2) {
            if (rx == 7) {
              pixelColor = lightBlue;
            } else {
              pixelColor = CRGB(qadd8(120, boost), qadd8(190, boost), 255);
            }
          } else if (ry == 3) {
            if (rx == 7) {
              pixelColor = mediumBlue;
            } else {
              pixelColor = CRGB(qadd8(80, boost), qadd8(160, boost), 255);
            }
          } else if (ry == 4) {
            if (rx == 7) {
              pixelColor = deepBlue;
            } else {
              pixelColor = mediumBlue;
            }
          } else if (ry == 5) {
            pixelColor = deepBlue;
          } else {
            pixelColor = deepBlue;
          }

          pset(x + rx, y + ry, pixelColor);
        }
      }
    }
  } else if (iconIndex == 3) {
    // Snowman: white body, red scarf
    CRGB red = CRGB(255, 0, 0);
    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          if (ry == 3 || (ry == 4 && rx == 6)) {
            pset(x + rx, y + ry, red);
          } else {
            pset(x + rx, y + ry, CRGB(255, 255, 255));
          }
        }
      }
    }
  } else if (iconIndex == 4) {
    // Thunderstorm: purple cloud, yellow lightning
    CRGB yellow = CRGB(255, 255, 0);
    for (uint8_t ry = 0; ry < 8; ++ry) {
      uint8_t row = pgm_read_byte(&WEATHER_ICONS[iconIndex][ry]);
      for (uint8_t rx = 0; rx < 8; ++rx) {
        if (row & (1 << (7 - rx))) {
          pset(x + rx, y + ry, (ry >= 4) ? yellow : c);
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

void drawWatchdogBootIcon() {
  clearAll();

  drawWeatherIcon(6, 12, 0, CRGB(255, 255, 255));

  // Gray glint on bottom-left pixel of each eye
  pset(12 + 1, 3, CRGB(100, 100, 100));
  pset(12 + 4, 3, CRGB(100, 100, 100));

  FastLED.setBrightness(80);
  FastLED.show();

  delay(2000);

  if (wdt_added) esp_task_wdt_reset();
}

void drawChristmasTree(int x, int y) {
  const CRGB G = CRGB(0, 200, 0);
  const CRGB D = CRGB(0, 80, 0);
  const CRGB Y = CRGB(255, 180, 0);
  const CRGB W = CRGB(255, 255, 255);
  const CRGB B = CRGB(100, 50, 10);

  pset(x+3,y+0,G); pset(x+4,y+0,G);
  pset(x+3,y+1,G); pset(x+4,y+1,Y);
  pset(x+2,y+2,D); pset(x+3,y+2,W); pset(x+4,y+2,G); pset(x+5,y+2,D);
  pset(x+1,y+3,D); pset(x+2,y+3,Y); pset(x+3,y+3,G); pset(x+4,y+3,G); pset(x+5,y+3,Y); pset(x+6,y+3,D);
  pset(x+2,y+4,G); pset(x+3,y+4,G); pset(x+4,y+4,G); pset(x+5,y+4,G);
  pset(x+1,y+5,D); pset(x+2,y+5,Y); pset(x+3,y+5,G); pset(x+4,y+5,W); pset(x+5,y+5,G); pset(x+6,y+5,G);
  pset(x+0,y+6,D); pset(x+1,y+6,W); pset(x+2,y+6,G); pset(x+3,y+6,G); pset(x+4,y+6,G); pset(x+5,y+6,Y); pset(x+6,y+6,G); pset(x+7,y+6,G);
  pset(x+3,y+7,B); pset(x+4,y+7,B);
}

void drawArrowIcon(uint8_t iconIndex, int x, int y, const CRGB& c) {
  if (iconIndex >= 2) return;
  for (uint8_t ry = 0; ry < 8; ++ry) {
    uint8_t row = pgm_read_byte(&ARROW_ICONS[iconIndex][ry]);
    for (uint8_t rx = 0; rx < 8; ++rx) {
      if (row & (1 << (7 - rx))) {
        pset(x + rx, y + ry, c);
      }
    }
  }
}

void drawBYULogo(int x, int y) {
  const CRGB B = CRGB(0, 51, 160);
  const CRGB W = CRGB(255, 255, 255);

  // Row 0
  pset(x+2,y+0,B); pset(x+3,y+0,B); pset(x+4,y+0,B); pset(x+5,y+0,B);
  pset(x+6,y+0,B); pset(x+7,y+0,B); pset(x+8,y+0,B); pset(x+9,y+0,B);
  pset(x+10,y+0,B); pset(x+11,y+0,B); pset(x+12,y+0,B); pset(x+13,y+0,B);
  // Row 1
  pset(x+0,y+1,B); pset(x+1,y+1,B); pset(x+2,y+1,B); pset(x+3,y+1,B);
  pset(x+4,y+1,W); pset(x+5,y+1,W); pset(x+6,y+1,B); pset(x+7,y+1,B);
  pset(x+8,y+1,B); pset(x+9,y+1,B); pset(x+10,y+1,W); pset(x+11,y+1,W);
  pset(x+12,y+1,B); pset(x+13,y+1,B); pset(x+14,y+1,B); pset(x+15,y+1,B);
  // Row 2
  pset(x+0,y+2,B); pset(x+1,y+2,B); pset(x+2,y+2,B); pset(x+3,y+2,W);
  pset(x+4,y+2,W); pset(x+5,y+2,W); pset(x+6,y+2,W); pset(x+7,y+2,B);
  pset(x+8,y+2,B); pset(x+9,y+2,W); pset(x+10,y+2,W); pset(x+11,y+2,W);
  pset(x+12,y+2,W); pset(x+13,y+2,B); pset(x+14,y+2,B); pset(x+15,y+2,B);
  // Row 3
  pset(x+0,y+3,B); pset(x+1,y+3,B); pset(x+2,y+3,B); pset(x+3,y+3,B);
  pset(x+4,y+3,B); pset(x+5,y+3,W); pset(x+6,y+3,W); pset(x+7,y+3,W);
  pset(x+8,y+3,W); pset(x+9,y+3,W); pset(x+10,y+3,W); pset(x+11,y+3,B);
  pset(x+12,y+3,B); pset(x+13,y+3,B); pset(x+14,y+3,B); pset(x+15,y+3,B);
  // Row 4
  pset(x+0,y+4,B); pset(x+1,y+4,B); pset(x+2,y+4,B); pset(x+3,y+4,B);
  pset(x+4,y+4,B); pset(x+5,y+4,B); pset(x+6,y+4,W); pset(x+7,y+4,W);
  pset(x+8,y+4,W); pset(x+9,y+4,W); pset(x+10,y+4,B); pset(x+11,y+4,B);
  pset(x+12,y+4,B); pset(x+13,y+4,B); pset(x+14,y+4,B); pset(x+15,y+4,B);
  // Row 5
  pset(x+0,y+5,B); pset(x+1,y+5,B); pset(x+2,y+5,B); pset(x+3,y+5,B);
  pset(x+4,y+5,B); pset(x+5,y+5,B); pset(x+6,y+5,W); pset(x+7,y+5,W);
  pset(x+8,y+5,W); pset(x+9,y+5,W); pset(x+10,y+5,B); pset(x+11,y+5,B);
  pset(x+12,y+5,B); pset(x+13,y+5,B); pset(x+14,y+5,B); pset(x+15,y+5,B);
  // Row 6
  pset(x+0,y+6,B); pset(x+1,y+6,B); pset(x+2,y+6,B); pset(x+3,y+6,B);
  pset(x+4,y+6,B); pset(x+5,y+6,W); pset(x+6,y+6,W); pset(x+7,y+6,W);
  pset(x+8,y+6,W); pset(x+9,y+6,W); pset(x+10,y+6,W); pset(x+11,y+6,B);
  pset(x+12,y+6,B); pset(x+13,y+6,B); pset(x+14,y+6,B); pset(x+15,y+6,B);
  // Row 7
  pset(x+2,y+7,B); pset(x+3,y+7,B); pset(x+4,y+7,B); pset(x+5,y+7,B);
  pset(x+6,y+7,B); pset(x+7,y+7,B); pset(x+8,y+7,B); pset(x+9,y+7,B);
  pset(x+10,y+7,B); pset(x+11,y+7,B); pset(x+12,y+7,B); pset(x+13,y+7,B);
}

void drawLSULogo(int x, int y) {
  // Tiger eye from reference image (16x8)
  const CRGB P = CRGB(70, 29, 124);    // LSU Purple #461D7C
  const CRGB G = CRGB(253, 208, 35);   // LSU Gold #FDD023
  const CRGB W = CRGB(255, 255, 255);  // White (eye whites)

  // Row 0: GGWWWPPPPPPPPPPW
  pset(x+0,y+0,G); pset(x+1,y+0,G);
  pset(x+2,y+0,W); pset(x+3,y+0,W); pset(x+4,y+0,W);
  pset(x+5,y+0,P); pset(x+6,y+0,P); pset(x+7,y+0,P); pset(x+8,y+0,P); pset(x+9,y+0,P);
  pset(x+10,y+0,P); pset(x+11,y+0,P); pset(x+12,y+0,P); pset(x+13,y+0,P); pset(x+14,y+0,P);
  pset(x+15,y+0,W);
  // Row 1: WGWWPPGGGPPPPGPP
  pset(x+0,y+1,W); pset(x+1,y+1,G); pset(x+2,y+1,W); pset(x+3,y+1,W);
  pset(x+4,y+1,P); pset(x+5,y+1,P);
  pset(x+6,y+1,G); pset(x+7,y+1,G); pset(x+8,y+1,G);
  pset(x+9,y+1,P); pset(x+10,y+1,P); pset(x+11,y+1,P); pset(x+12,y+1,P);
  pset(x+13,y+1,G);
  pset(x+14,y+1,P); pset(x+15,y+1,P);
  // Row 2: WWWPPGGGGPPWPGGP
  pset(x+0,y+2,W); pset(x+1,y+2,W); pset(x+2,y+2,W);
  pset(x+3,y+2,P); pset(x+4,y+2,P);
  pset(x+5,y+2,G); pset(x+6,y+2,G); pset(x+7,y+2,G); pset(x+8,y+2,G);
  pset(x+9,y+2,P); pset(x+10,y+2,P);
  pset(x+11,y+2,W);
  pset(x+12,y+2,P);
  pset(x+13,y+2,G); pset(x+14,y+2,G);
  pset(x+15,y+2,P);
  // Row 3: WPPPPGGGGPPWPGGP
  pset(x+0,y+3,W);
  pset(x+1,y+3,P); pset(x+2,y+3,P); pset(x+3,y+3,P); pset(x+4,y+3,P);
  pset(x+5,y+3,G); pset(x+6,y+3,G); pset(x+7,y+3,G); pset(x+8,y+3,G);
  pset(x+9,y+3,P); pset(x+10,y+3,P);
  pset(x+11,y+3,W);
  pset(x+12,y+3,P);
  pset(x+13,y+3,G); pset(x+14,y+3,G);
  pset(x+15,y+3,P);
  // Row 4: WWWPPGGGGPPPPGGP
  pset(x+0,y+4,W); pset(x+1,y+4,W); pset(x+2,y+4,W);
  pset(x+3,y+4,P); pset(x+4,y+4,P);
  pset(x+5,y+4,G); pset(x+6,y+4,G); pset(x+7,y+4,G); pset(x+8,y+4,G);
  pset(x+9,y+4,P); pset(x+10,y+4,P); pset(x+11,y+4,P); pset(x+12,y+4,P);
  pset(x+13,y+4,G); pset(x+14,y+4,G);
  pset(x+15,y+4,P);
  // Row 5: WGWWPPGGGGGGGGPP
  pset(x+0,y+5,W); pset(x+1,y+5,G); pset(x+2,y+5,W); pset(x+3,y+5,W);
  pset(x+4,y+5,P); pset(x+5,y+5,P);
  pset(x+6,y+5,G); pset(x+7,y+5,G); pset(x+8,y+5,G); pset(x+9,y+5,G);
  pset(x+10,y+5,G); pset(x+11,y+5,G); pset(x+12,y+5,G); pset(x+13,y+5,G);
  pset(x+14,y+5,P); pset(x+15,y+5,P);
  // Row 6: WGGWWPPPPPPPPPPW
  pset(x+0,y+6,W); pset(x+1,y+6,G); pset(x+2,y+6,G); pset(x+3,y+6,W); pset(x+4,y+6,W);
  pset(x+5,y+6,P); pset(x+6,y+6,P); pset(x+7,y+6,P); pset(x+8,y+6,P); pset(x+9,y+6,P);
  pset(x+10,y+6,P); pset(x+11,y+6,P); pset(x+12,y+6,P); pset(x+13,y+6,P); pset(x+14,y+6,P);
  pset(x+15,y+6,W);
  // Row 7: GGWWWWWWWWWWWWWW
  pset(x+0,y+7,G); pset(x+1,y+7,G);
  pset(x+2,y+7,W); pset(x+3,y+7,W); pset(x+4,y+7,W); pset(x+5,y+7,W); pset(x+6,y+7,W);
  pset(x+7,y+7,W); pset(x+8,y+7,W); pset(x+9,y+7,W); pset(x+10,y+7,W); pset(x+11,y+7,W);
  pset(x+12,y+7,W); pset(x+13,y+7,W); pset(x+14,y+7,W); pset(x+15,y+7,W);
}
