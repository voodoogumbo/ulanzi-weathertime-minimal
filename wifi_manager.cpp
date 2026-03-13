/*
  wifi_manager.cpp - WiFi management and weather API implementations
*/

#include "wifi_manager.h"
#include "config.h"
#include "mqtt_handler.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

// Externs for network operations
extern SemaphoreHandle_t displayMutex;
extern float temperature;
extern char weatherCondition[];
extern char weatherIcon[];
extern volatile bool displayDirty;

// Module-local HTTP client
static HTTPClient http;

void ensureWifi() {
  if (WiFi.status() == WL_CONNECTED) return;

  mqtt.disconnect();  // Invalidate dead socket before WiFi reconnect
  Serial.print("WiFi connecting");
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

  unsigned long startAttemptTime = millis();

  while (WiFi.status() != WL_CONNECTED && millis() - startAttemptTime < 5000) {
    esp_task_wdt_reset();
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println(" Failed! Will retry next cycle.");
    return;
  }

  Serial.printf(" WiFi connected! IP: %s, RSSI: %d dBm\n",
                WiFi.localIP().toString().c_str(), WiFi.RSSI());
}

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
  http.setTimeout(3000);
  http.setConnectTimeout(2000);
  http.setReuse(false);
  esp_task_wdt_reset();
  int httpCode = http.GET();
  esp_task_wdt_reset();
  Serial.printf("Weather API HTTP response: %d\n", httpCode);

  if (httpCode == HTTP_CODE_OK) {
    static StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, http.getStream());

    if (!error) {
      float t = doc["main"]["temp"];
      char cond[24], icon[8];
      strncpy(cond, doc["weather"][0]["main"] | "", sizeof(cond) - 1);
      cond[sizeof(cond) - 1] = '\0';
      strncpy(icon, doc["weather"][0]["icon"] | "", sizeof(icon) - 1);
      icon[sizeof(icon) - 1] = '\0';
      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(50))) {
        temperature = t;
        memcpy(weatherCondition, cond, sizeof(cond));
        memcpy(weatherIcon, icon, sizeof(icon));
        displayDirty = true;
        xSemaphoreGive(displayMutex);
      }
      Serial.printf("Weather updated: %.1fF, %s, icon=%s\n", t, cond, icon);
    } else {
      Serial.printf("Weather JSON parse error: %s\n", error.c_str());
    }
  } else {
    Serial.printf("Weather API request failed: HTTP %d\n", httpCode);
  }
  http.end();
}
