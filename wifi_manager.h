/*
  wifi_manager.h - WiFi management and weather API
  NOTE: Named wifi_manager (not network) to avoid case-insensitive
  collision with ESP32 platform's Network.h
*/

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

// Ensure WiFi connection (reconnect if needed)
void ensureWifi();

// Fetch weather data from OpenWeather API
void fetchWeatherData();

#endif // WIFI_MANAGER_H
