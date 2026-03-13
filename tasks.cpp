/*
  tasks.cpp - FreeRTOS task implementations
*/

#include "tasks.h"
#include "display.h"
#include "time_helpers.h"
#include "pages.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "chime.h"
#include "hw_config.h"
#include "config.h"
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

// Externs for task operations
extern SemaphoreHandle_t displayMutex;
extern volatile bool notifyActive;
extern volatile bool notifyScrolling;
extern volatile bool colonVisible;
extern volatile bool displayDirty;
extern PageType currentPage;
extern bool rotationEnabled;
extern unsigned long pageDurationMs;
extern volatile bool christmasMode;
extern unsigned long lastChristmasScrollMs;
extern unsigned long notifyEndMs;
extern volatile bool weatherFetchRequested;
extern unsigned long weatherUpdateIntervalMs;
extern int lastChimeHour;

void renderTask(void *pvParameters) {
  unsigned long lastRenderMs = 0;
  unsigned long lastBlinkMs = 0;
  unsigned long lastPageChangeMs = 0;

  Serial.println("Render task started on Core 1");

  while (true) {
    unsigned long currentMs = millis();
    esp_task_wdt_reset();

    // Colon blinking (500ms)
    if (currentMs - lastBlinkMs > 500) {
      lastBlinkMs = currentMs;
      colonVisible = !colonVisible;
      displayDirty = true;
    }

    // Page rotation
    if (rotationEnabled && !notifyActive && (currentMs - lastPageChangeMs > pageDurationMs)) {
      lastPageChangeMs = currentMs;
      currentPage = (currentPage == PAGE_CLOCK) ? PAGE_WEATHER : PAGE_CLOCK;
      displayDirty = true;
    }

    // Christmas periodic scroll
    if (christmasMode && !notifyActive &&
        (currentMs - lastChristmasScrollMs >= CHRISTMAS_SCROLL_INTERVAL_MS)) {
      lastChristmasScrollMs = currentMs;
      triggerChristmasScroll();
    }

    // Scrolling notifications are always dirty
    if (notifyActive && notifyScrolling) displayDirty = true;

    // Adaptive frame rate: 20 FPS scrolling, 10 FPS otherwise
    unsigned long frameInterval = (notifyActive && notifyScrolling) ? 50 : 100;
    if (displayDirty && (currentMs - lastRenderMs >= frameInterval)) {
      lastRenderMs = currentMs;
      displayDirty = false;

      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(10))) {
        updateBrightness();

        // Update Christmas mode from current date
        {
          struct tm timeCheck;
          if (getCurrentTime(&timeCheck)) {
            christmasMode = isChristmasSeason(timeCheck);
          }
        }

        if (notifyActive) {
          drawNotify();
          if (!notifyScrolling && notifyEndMs > 0 && (long)(currentMs - notifyEndMs) >= 0) {
            notifyActive = false;
            displayDirty = true;
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

    // Hourly chime (outside display mutex)
    {
      struct tm chimeTime;
      if (getCurrentTime(&chimeTime) && chimeTime.tm_hour != lastChimeHour) {
        lastChimeHour = chimeTime.tm_hour;
        int minuteOfDay = chimeTime.tm_hour * 60 + chimeTime.tm_min;
        if (minuteOfDay >= 390 && minuteOfDay < 1320) {
          triggerChime();
        }
      }
    }
    updateChime();

    vTaskDelay(pdMS_TO_TICKS(1));
  }
}

void networkTask(void *pvParameters) {
  unsigned long lastWeatherCheck = 0;

  Serial.println("Network task started on Core 0");

  while (true) {
    esp_task_wdt_reset();

    ensureWifi();
    ArduinoOTA.handle();
    ensureMqtt();
    mqtt.loop();

    esp_task_wdt_reset();

    unsigned long currentMs = millis();
    if (weatherFetchRequested || (currentMs - lastWeatherCheck > weatherUpdateIntervalMs)) {
      weatherFetchRequested = false;
      lastWeatherCheck = currentMs;
      fetchWeatherData();
      esp_task_wdt_reset();
    }

    vTaskDelay(pdMS_TO_TICKS(100));
  }
}
