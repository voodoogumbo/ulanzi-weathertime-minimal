/*
  TC001 Enhanced Clock - Modular firmware for Ulanzi TC001
  Features: AWTRIX-style display, MQTT time sync, weather integration,
  cloud gradients, dual-core FreeRTOS, Mountain Time DST support,
  optional MQTT TLS encryption

  Setup: Copy config.h.example -> config.h, configure, upload
  MQTT Topics: notify, page, config, weather, time, christmas
*/

#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <FastLED.h>
#include <HTTPClient.h>
#include <time.h>
#include <ArduinoOTA.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "hw_config.h"
#include "font_data.h"
#include "display.h"
#include "time_helpers.h"
#include "font_render.h"
#include "graphics.h"
#include "pages.h"
#include "wifi_manager.h"
#include "mqtt_handler.h"
#include "chime.h"
#include "tasks.h"

// ==================== Network Clients ====================

#if MQTT_USE_TLS
  #include <WiFiClientSecure.h>
  WiFiClientSecure espClient;
#else
  WiFiClient espClient;
#endif

PubSubClient mqtt(espClient);

// ==================== LED Array ====================

CRGB leds[NUM_LEDS];

// ==================== Page System State ====================

PageType currentPage = PAGE_CLOCK;
bool rotationEnabled = true;
unsigned long pageDurationMs = PAGE_DURATION_MS;

// ==================== Notification State ====================

volatile bool notifyActive = false;
char notifyText[64] = {0};
CRGB notifyColor = CRGB(255, 255, 0);
unsigned long notifyEndMs = 0;
int notifyScrollOffset = 0;
unsigned long lastScrollMs = 0;
uint16_t scrollSpeedMs = 80;
int notifyTextWidth = 0;
volatile bool notifyScrolling = false;
uint8_t notifyScrollLoops = 0;
uint8_t notifyScrollMaxLoops = 2;

// ==================== Weather Data ====================

char weatherCondition[24] = {0};
char weatherIcon[8] = {0};
float temperature = 0.0;
unsigned long weatherUpdateIntervalMs = WEATHER_UPDATE_INTERVAL_MS;

// ==================== Animation State ====================

volatile bool colonVisible = true;
volatile bool displayDirty = true;

// ==================== Brightness Control ====================

uint8_t currentBrightness = BRIGHTNESS_DAY;

// ==================== MQTT Time Management ====================

volatile bool weatherFetchRequested = false;
volatile bool mqttTimeAvailable = false;
volatile time_t mqttUnixTime = 0;
volatile unsigned long mqttTimeMs = 0;

// ==================== FreeRTOS ====================

TaskHandle_t renderTaskHandle = NULL;
TaskHandle_t networkTaskHandle = NULL;
SemaphoreHandle_t displayMutex = NULL;

// ==================== Watchdog State ====================

bool wdt_added = false;

// ==================== Christmas Mode ====================

volatile bool christmasMode = false;
unsigned long lastChristmasScrollMs = 0;
volatile bool notifyAlternateColors = false;
volatile bool notifyBYUMode = false;
volatile bool notifyLSUMode = false;
volatile uint8_t notifyIconType = 0;  // 0=none, 1=arrow_down, 2=arrow_up

// ==================== Hourly Chime ====================

uint8_t chimePhase = 0;
unsigned long chimePhaseMs = 0;
int lastChimeHour = -1;

// ==================== Setup ====================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TC001 Enhanced Clock Starting ===");

  // Create display mutex first
  displayMutex = xSemaphoreCreateMutex();
  if (displayMutex == NULL) {
    Serial.println("Failed to create display mutex!");
    while (1) delay(1000);
  }

  // Initialize LED matrix
  Serial.println("Initializing LED matrix...");
  FastLED.addLeds<LED_TYPE, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);

  // Initialize buzzer PWM
  ledcAttach(BUZZER_PIN, 1000, 8);
  ledcWriteTone(BUZZER_PIN, 0);

  // Show watchdog boot icon
  Serial.println("Showing watchdog boot icon...");
  drawWatchdogBootIcon();

  // Set initial brightness
  updateBrightness();
  clearAll();
  FastLED.show();
  Serial.printf("LED matrix initialized with brightness: %d\n", currentBrightness);

  // Setup timezone (Mountain Time with DST)
  Serial.println("Setting up Mountain Time timezone...");
  const char* TZ_DENVER = "MST7MDT,M3.2.0/2,M11.1.0/2";
  setenv("TZ", TZ_DENVER, 1);
  tzset();
  Serial.println("Timezone configured: Mountain Time (MST/MDT with automatic DST)");

  // Initialize network
  ensureWifi();

  // Configure OTA updates
  ArduinoOTA.setHostname("tc001-clock");
  ArduinoOTA.setPassword(OTA_PASSWORD);
  ArduinoOTA.onStart([]() {
    Serial.println("OTA update starting...");
    if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
      clearAll();
      drawLargeString("BEEP", 2, 0, CRGB(0, 255, 255));
      FastLED.show();
      xSemaphoreGive(displayMutex);
    }
  });
  ArduinoOTA.onEnd([]() { Serial.println("\nOTA update complete!"); });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    esp_task_wdt_reset();
    uint8_t pct = (progress * 100) / total;
    if (pct == 50) {
      if (xSemaphoreTake(displayMutex, pdMS_TO_TICKS(100))) {
        clearAll();
        drawLargeString("BOOP", 2, 0, CRGB(255, 0, 255));
        FastLED.show();
        xSemaphoreGive(displayMutex);
      }
    }
  });
  ArduinoOTA.onError([](ota_error_t error) { Serial.printf("OTA error: %u\n", error); });
  ArduinoOTA.begin();
  Serial.println("OTA ready (tc001-clock)");

  // Allow network stack to stabilize
  Serial.println("Allowing network to stabilize...");
  for (int i = 0; i < 10; i++) {
    delay(500);
    Serial.print(".");
  }
  Serial.println(" Network ready!");

  // Configure MQTT TLS if enabled
  #if MQTT_USE_TLS
    espClient.setCACert(MQTT_CA_CERT);
    Serial.println("MQTT TLS: CA certificate configured");
    espClient.setHandshakeTimeout(5);
    espClient.setTimeout(5000);  // socket-level timeout in ms
    Serial.println("MQTT TLS: timeouts set to 5s");
  #endif

  // Configure MQTT
  Serial.printf("Configuring MQTT for server %s:%d\n", MQTT_HOST, MQTT_PORT);
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(20);
  #if MQTT_USE_TLS
    mqtt.setSocketTimeout(10);  // TLS handshake needs more time
  #else
    mqtt.setSocketTimeout(3);
  #endif
  mqtt.setCallback(mqttCallback);

  // Initial weather fetch
  Serial.println("Fetching initial weather data...");
  fetchWeatherData();

  // Configure watchdog timer
  if (!wdt_added) {
    Serial.println("Configuring watchdog timer for dual-core tasks...");
    esp_task_wdt_config_t twdt_config = {
      .timeout_ms = 30000,
      .idle_core_mask = 0,
      .trigger_panic = true
    };
    esp_task_wdt_init(&twdt_config);
    wdt_added = true;
    Serial.println("Watchdog timer configured for tasks only.");
  }

  // Create FreeRTOS tasks
  Serial.println("Creating FreeRTOS tasks...");

  // Render task on Core 1 (higher priority)
  xTaskCreatePinnedToCore(
    renderTask, "RenderTask", 4096, NULL, 2, &renderTaskHandle, 1
  );

  // Network task on Core 0 (lower priority, larger stack for TLS)
  xTaskCreatePinnedToCore(
    networkTask, "NetworkTask",
    #if MQTT_USE_TLS
      10240,  // larger stack for TLS handshake
    #else
      8192,
    #endif
    NULL, 1, &networkTaskHandle, 0
  );

  // Add tasks to watchdog
  if (renderTaskHandle != NULL) {
    esp_task_wdt_add(renderTaskHandle);
    Serial.println("Render task created and added to watchdog (Core 1)");
  }
  if (networkTaskHandle != NULL) {
    esp_task_wdt_add(networkTaskHandle);
    Serial.println("Network task created and added to watchdog (Core 0)");
  }

  Serial.println("=== Dual-core setup complete ===");
}

// ==================== Loop ====================

void loop() {
  vTaskDelay(portMAX_DELAY);
}
