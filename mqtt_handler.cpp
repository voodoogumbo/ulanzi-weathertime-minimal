/*
  mqtt_handler.cpp - MQTT connection management and message handler implementations
*/

#include "mqtt_handler.h"
#include "config.h"
#include "hw_config.h"
#include "font_render.h"
#include "time_helpers.h"
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// Externs for MQTT operations
extern SemaphoreHandle_t displayMutex;
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
extern volatile bool weatherFetchRequested;
extern volatile bool displayDirty;
extern PageType currentPage;
extern bool rotationEnabled;
extern unsigned long pageDurationMs;
extern unsigned long weatherUpdateIntervalMs;
extern volatile bool christmasMode;

// Module-local MQTT buffers
static char mqttJsonBuf[512];
static StaticJsonDocument<512> mqttDoc;

// Forward declarations for handlers
static void handleNotify(const JsonDocument& doc);
static void handlePageCommand(const JsonDocument& doc);
static void handleConfig(const JsonDocument& doc);
static void handleTimeCommand(const JsonDocument& doc);

// Helper function to subscribe to a topic
static bool mqttSubscribe(const char* suffix, bool& allSubscribed, uint8_t qos = 0) {
  char topic[128];
  snprintf(topic, 128, "%s%s", MQTT_BASE, suffix);
  if (mqtt.subscribe(topic, qos)) {
    Serial.printf("Subscribed: %s (QoS %d)\n", topic, qos);
    return true;
  } else {
    Serial.printf("Subscribe failed: %s\n", topic);
    allSubscribed = false;
    return false;
  }
}

void ensureMqtt() {
  if (mqtt.connected()) { return; }

  static unsigned long lastAttempt = 0;
  static unsigned long backoffMs = 0;
  if (backoffMs > 0 && (millis() - lastAttempt) < backoffMs) return;
  lastAttempt = millis();

  esp_task_wdt_reset();

  char cid[24];
  snprintf(cid, 24, "tc001-%llX", ESP.getEfuseMac());
  Serial.printf("MQTT connecting to %s:%d...\n", MQTT_HOST, MQTT_PORT);

  #if MQTT_USE_TLS
    Serial.printf("Free heap: %u bytes\n", ESP.getFreeHeap());
    esp_task_wdt_reset();
  #endif

  bool connected;
  if (strlen(MQTT_USER) > 0) {
    connected = mqtt.connect(cid, MQTT_USER, MQTT_PASS);
  } else {
    connected = mqtt.connect(cid);
  }
  esp_task_wdt_reset();  // Reset WDT immediately after TLS handshake attempt

  if (connected) {
    backoffMs = 0;
    Serial.println("MQTT connected!");

    bool allSubscribed = true;
    mqttSubscribe("/notify", allSubscribed);
    mqttSubscribe("/page", allSubscribed);
    mqttSubscribe("/config", allSubscribed);
    mqttSubscribe("/weather", allSubscribed);
    mqttSubscribe("/time", allSubscribed, 1);
    mqttSubscribe("/christmas", allSubscribed);

    if (!allSubscribed)
      Serial.println("Some MQTT subscriptions failed");
  } else {
    esp_task_wdt_reset();
    backoffMs = (backoffMs == 0) ? 2000UL : min(backoffMs * 2, 30000UL);
    Serial.printf("MQTT failed (err %d), retry in %lus\n", mqtt.state(), backoffMs / 1000);
  }
}

void mqttCallback(char* topic, byte* payload, unsigned int len) {
  esp_task_wdt_reset();

  if (len > 500) return;

  memcpy(mqttJsonBuf, payload, len);
  mqttJsonBuf[len] = '\0';

  Serial.printf("MQTT: %s (%d bytes)\n", topic, len);

  DeserializationError err = deserializeJson(mqttDoc, mqttJsonBuf);
  if (err) {
    Serial.printf("JSON error: %s - payload: %s\n", err.c_str(), mqttJsonBuf);
    return;
  }

  if (strstr(topic, "/notify")) {
    handleNotify(mqttDoc);
  } else if (strstr(topic, "/page")) {
    handlePageCommand(mqttDoc);
  } else if (strstr(topic, "/config")) {
    handleConfig(mqttDoc);
  } else if (strstr(topic, "/weather")) {
    weatherFetchRequested = true;
  } else if (strstr(topic, "/time")) {
    handleTimeCommand(mqttDoc);
  } else if (strstr(topic, "/christmas")) {
    if (mqttDoc.containsKey("scroll") || mqttDoc.containsKey("test")) {
      triggerChristmasScroll();
    }
  }
}

void triggerChristmasScroll() {
  const char* text = "MERRY CHRISTMAS";
  int textWidth = getLargeStringWidth(text);

  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
    strncpy(notifyText, text, 63);
    notifyText[63] = '\0';
    notifyColor = CHRISTMAS_GREEN;
    scrollSpeedMs = 80;
    notifyScrollMaxLoops = 1;
    notifyTextWidth = textWidth;
    notifyScrolling = true;
    notifyScrollOffset = MW;
    notifyScrollLoops = 0;
    lastScrollMs = millis();
    notifyEndMs = 0;
    notifyAlternateColors = true;
    notifyBYUMode = false;
    notifyLSUMode = false;
    notifyActive = true;
    displayDirty = true;
    xSemaphoreGive(displayMutex);
  }
  Serial.println("Christmas scroll triggered!");
}

// Handle notification display command
static void handleNotify(const JsonDocument& doc) {
  char text[64];
  strncpy(text, doc["text"] | "", sizeof(text) - 1);
  text[sizeof(text) - 1] = '\0';
  const char* hex = doc["color"] | "FFFF00";
  unsigned long v = strtoul(hex, nullptr, 16);
  CRGB color = CRGB((v>>16)&0xFF, (v>>8)&0xFF, v&0xFF);
  uint16_t dur = doc["duration"] | 4;
  uint16_t speed = doc["speed"] | 80;
  uint8_t maxLoops = doc["repeat"] | 2;
  int largeWidth = getLargeStringWidth(text);

  // Parse optional icon field
  const char* iconStr = doc["icon"] | "";
  uint8_t iconType = 0;
  if (strcmp(iconStr, "arrow_down") == 0) iconType = 1;
  else if (strcmp(iconStr, "arrow_up") == 0) iconType = 2;

  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
    memcpy(notifyText, text, 64);
    notifyColor = color;
    scrollSpeedMs = speed;
    notifyScrollMaxLoops = maxLoops;
    notifyIconType = iconType;

    if (largeWidth > MW) {
      notifyTextWidth = largeWidth;
      notifyScrolling = true;
      notifyScrollOffset = MW;
      notifyScrollLoops = 0;
      lastScrollMs = millis();
      notifyEndMs = 0;
    } else {
      notifyTextWidth = getSmallStringWidth(text);
      notifyScrolling = false;
      notifyEndMs = millis() + dur * 1000UL;
    }
    notifyAlternateColors = false;
    notifyBYUMode = (strncmp(text, "BYU", 3) == 0);
    notifyLSUMode = (!notifyBYUMode && strncmp(text, "LSU", 3) == 0);
    if ((notifyBYUMode || notifyLSUMode) && notifyScrolling) {
      notifyTextWidth = 18 + largeWidth;  // 16px logo + 2px gap
    } else if (iconType > 0 && notifyScrolling) {
      notifyTextWidth = 10 + largeWidth;  // 8px icon + 2px gap + text
    }
    notifyActive = true;
    displayDirty = true;
    xSemaphoreGive(displayMutex);
  }

  if (largeWidth > MW) {
    Serial.printf("Scroll notify: \"%s\" (%dpx wide, %d loops, %dms/px)\n",
                  text, largeWidth, maxLoops, speed);
  }
}

// Handle page change command
static void handlePageCommand(const JsonDocument& doc) {
  const char* page = doc["page"] | "";
  PageType newPage = currentPage;
  if (strcmp(page, "clock") == 0) newPage = PAGE_CLOCK;
  else if (strcmp(page, "weather") == 0) newPage = PAGE_WEATHER;
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
    currentPage = newPage;
    displayDirty = true;
    xSemaphoreGive(displayMutex);
  }
}

// Handle general configuration settings
static void handleConfig(const JsonDocument& doc) {
  unsigned long newPageDur = pageDurationMs;
  bool newRotation = rotationEnabled;
  unsigned long newWeatherInt = weatherUpdateIntervalMs;
  if (doc.containsKey("page_duration"))
    newPageDur = (doc["page_duration"].as<uint16_t>()) * 1000UL;
  if (doc.containsKey("rotation_enabled"))
    newRotation = doc["rotation_enabled"];
  if (doc.containsKey("weather_update_minutes"))
    newWeatherInt = (doc["weather_update_minutes"].as<uint16_t>()) * 60 * 1000UL;
  if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
    pageDurationMs = newPageDur;
    rotationEnabled = newRotation;
    weatherUpdateIntervalMs = newWeatherInt;
    xSemaphoreGive(displayMutex);
  }
}

// Handle MQTT time updates
static void handleTimeCommand(const JsonDocument& doc) {
  if (doc.containsKey("unix_time")) {
    time_t t = doc["unix_time"].as<time_t>();
    unsigned long ms = millis();
    // Update time triple under spinlock for cross-core consistency
    taskENTER_CRITICAL(&timeMux);
    mqttUnixTime = t;
    mqttTimeMs = ms;
    mqttTimeAvailable = true;
    taskEXIT_CRITICAL(&timeMux);
    displayDirty = true;
    Serial.printf("MQTT time received: %lu (Unix timestamp)\n", t);
  }
}
