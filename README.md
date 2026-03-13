# TC001 Enhanced Clock 🕐

A feature-rich smart clock firmware for the Ulanzi TC001 32x8 LED matrix with AWTRIX-style display, scrolling notifications, live BYU sports scores, NWS weather alerts, and MQTT control.

## 🚀 What's New

### Latest Additions
- 🔔 **Hourly Chime**: Two-note C5→E5 buzzer chime on the hour (daytime only)
- 🔄 **OTA Updates**: Wireless firmware updates with password protection and visual progress
- 🎄 **Christmas Mode**: Auto-activates Dec 20-26 with themed clock, tree icon, and periodic scroll
- 🏈 **BYU Logo Mode**: Notifications starting with "BYU" auto-prepend the full 16x8 oval logo

### Firmware Optimizations
- ⚡ **Cached Recycling Check**: Was 60 `mktime()` calls/sec, now rechecks once per hour
- 🧹 **Zero Heap Fragmentation**: All Arduino `String` replaced with fixed `char[]` buffers for long-term stability
- 🎯 **Adaptive Frame Rate**: 10 FPS normally (was 30), 20 FPS during scroll — 66% less CPU
- 🛡️ **MQTT Payload Guard**: Rejects oversized payloads to prevent stack overflow
- 🌡️ **Negative Temperature Support**: Renders minus sign for sub-zero Fahrenheit

### Scrolling Notifications
- 📜 **5x7 Large Font**: Full A-Z alphabet at clock-digit height for readable scrolling text
- 🔄 **Auto-Scroll**: Text wider than 32px automatically scrolls with configurable speed and repeat
- 📏 **Smart Sizing**: Short text uses large font centered, falls back to 3x5 if needed
- ⚙️ **MQTT Params**: `speed` (ms/pixel), `repeat` (loop count), `color` (hex)

### Smart Alerts (Python Script)
- 🏈 **BYU Live Scores**: ESPN API — game day alerts, live score updates every 2 min, final scores
- 🏆 **Win Celebration**: BYU wins scroll every 10 min for 1 hour in green. Losses scroll once in red.
- 🥶 **Freeze Warnings**: Scrolls ice-blue alert when temp drops below 32F
- 🌪️ **NWS Severe Weather**: Tornado, winter storm, flood warnings — free API, no key needed
- ♻️ **Recycling Reminder**: "RECYCLING TOMORROW" scrolls Sunday evening before pickup

## ✨ Features

### 🔥 Core Display
- **32x8 LED Matrix**: Full-width AWTRIX TMODE5-style clock display
- **Dual Font System**: 6x7 bold digits for clock, 5x7 letters for scrolling notifications
- **Enhanced Colon**: Two 2x2 blocks with clean blinking animation
- **Page Rotation**: Clock → Weather (configurable timing)
- **Ultra-Dim Night Mode**: Red display + brightness = 1 after 10pm
- **LaMetric Watchdog Boot**: Startup icon in signature orange

### 🌤️ Weather
- **OpenWeather API**: Real-time data with 15-minute updates
- **Day/Night Icons**: Sun/moon switching based on API data
- **3D Cloud Gradients**: 5-row gradient system with realistic depth
- **Temperature Color Scaling**: Ice-blue (sub-zero) → white (0F) → red (100F+)
- **Negative Temps**: Proper minus sign rendering for sub-zero weather
- **Weather Icon Colors**:
  - ☀️ Sun: Golden yellow — ☁️ Clouds: Multi-gray gradient
  - 🌙 Moon: Silver-blue — 🌧️ Rain: Deep blue
  - ❄️ Snow: Pure white — ⛈️ Thunder: Electric purple

### 📡 MQTT
- **MQTT Time Sync**: Unix timestamp with automatic Mountain Time/DST
- **Zero-Allocation Routing**: `strstr()` topic matching, no heap allocations
- **Smart Status Indicator**: Royal blue pixel for MQTT, green on recycle days
- **Scrolling Notifications**: Long text auto-scrolls with configurable speed/repeat
- **Payload Protection**: 500-byte cap prevents stack overflow from malformed messages

### 🔔 Hourly Chime
- **Two-Note Chime**: C5 → E5 buzzer chime on the hour (GPIO15 piezo)
- **Daytime Only**: Active 6:30 AM – 10:00 PM (respects night mode)
- **Non-Blocking**: State-machine driven, never stalls the display

### 🔄 OTA Updates
- **ArduinoOTA**: Wireless firmware updates over WiFi (no USB needed after first flash)
- **Password Protected**: Configurable OTA password in `config.h`
- **Visual Progress**: "BEEP" at start, "BOOP" at 50% on the display

### 🔐 MQTT TLS Encryption (Optional)
- **WiFiClientSecure**: Drop-in encrypted connection to MQTT broker
- **Disabled by Default**: Set `MQTT_USE_TLS 1` in config.h to enable
- **CA Certificate**: Supply PEM cert for full verification, or leave empty for `setInsecure()` mode
- **Authentication**: Optional `MQTT_USER`/`MQTT_PASS` for broker auth (works with or without TLS)

### 🔧 Architecture
- **Modular Design**: .h/.cpp module pairs (11 modules) compiled by Arduino IDE automatically
- **Dual-Core FreeRTOS**: Core 1 renders, Core 0 handles networking (no scroll stutter)
- **15-Second Watchdog**: Task-level monitoring (idle cores excluded to prevent false triggers)
- **Adaptive FPS**: 10 FPS for static display, 20 FPS during scroll animations
- **No Arduino String**: Fixed buffers eliminate heap fragmentation over weeks of uptime

## 🛠️ Hardware

- **Ulanzi TC001** (32x8 WS2812B LED matrix, ESP32-D0WD)
- **LED Matrix**: GPIO32, serpentine wiring (even rows L→R, odd rows R→L)
- **Light Sensor**: GPIO35 (GL5516 photoresistor)
- **Buttons**: Left (GPIO26), Middle (GPIO27), Right (GPIO14)
- **USB**: CH340 USB-to-Serial
- **Also available** (unused): SHT3x temp sensor (I2C 0x44), DS1307 RTC (I2C 0x68), Buzzer (GPIO15)

## 🚀 Quick Setup

### 1. Clone & Configure
```bash
git clone https://github.com/voodoogumbo/ulanzi-weathertime-minimal.git
cd ulanzi-weathertime-minimal
cp config.h.example config.h
```

### 2. Edit `config.h`
```cpp
#define WIFI_SSID       "YourWiFiNetwork"
#define WIFI_PASSWORD   "YourWiFiPassword"
#define MQTT_HOST       "192.168.1.100"
#define MQTT_PORT       1883
#define MQTT_BASE       "tc001"
#define OTA_PASSWORD    "your_ota_password"
#define OPENWEATHER_API_KEY "your_api_key_here"
#define OPENWEATHER_LAT "YOUR_LATITUDE"
#define OPENWEATHER_LON "YOUR_LONGITUDE"
```

### 3. Install Arduino Libraries
In Arduino IDE → Sketch → Include Library → Manage Libraries:
- `FastLED` by Daniel Garcia
- `PubSubClient` by Nick O'Leary
- `ArduinoJson` by Benoit Blanchon

### 4. Arduino IDE Board Settings
```
Board:            ESP32 Dev Module
Upload Speed:     115200  (CRITICAL — higher speeds fail on TC001)
CPU Frequency:    240MHz
Flash Frequency:  80MHz
Flash Mode:       QIO
Flash Size:       4MB (32Mb)
Partition Scheme: Default 4MB with spiffs (1.9MB APP/190KB SPIFFS)
Core Debug Level: None
PSRAM:            Disabled
```

### 5. Flash the TC001
1. **Unplug** USB from the TC001
2. **Hold the middle button** (center front)
3. **While holding**, plug in USB
4. Hold 2-3 more seconds, then release
5. Click **Upload** in Arduino IDE

Success: orange dog face icon → clock with royal blue status dot.

### 6. Setup Python Script
The Python script handles time sync and all smart alerts:

```bash
pip install paho-mqtt pytz
```

Copy the example and fill in your values:
```bash
cp mqtt_time_recycling_publisher.py.example mqtt_time_recycling_publisher.py
```

Edit the config section in `mqtt_time_recycling_publisher.py`:
```python
MQTT_HOST = "192.168.1.100"       # Your MQTT broker
OPENWEATHER_API_KEY = "your_key"  # For freeze warnings
OPENWEATHER_LAT = "YOUR_LATITUDE"  # Your location
OPENWEATHER_LON = "YOUR_LONGITUDE"
```

Run it (or set up as a scheduled task / service):
```bash
python3 mqtt_time_recycling_publisher.py
```

The script automatically provides:
- Time sync every 60 seconds (retained for instant reboot recovery)
- Freeze warnings when temp < 32F (checks every 30 min, alerts every 10 min)
- NWS severe weather alerts (checks every 15 min)
- Recycling reminders Sunday evening (every 10 min after 6 PM)
- BYU basketball & football live scores (ESPN API, every 2 min during games)

## 📡 MQTT Topics & Commands

Replace `tc001` with your configured `MQTT_BASE` value.

### 📜 Notifications (with Scrolling)
```bash
# Short text — displays centered in large 5x7 font
mosquitto_pub -h BROKER -t "tc001/notify" -m '{"text":"Hello","color":"00FF00"}'

# Long text — auto-scrolls across the display
mosquitto_pub -h BROKER -t "tc001/notify" -m '{"text":"BYU beats Arizona in overtime thriller","color":"00FF00"}'

# Custom scroll speed and repeat count
mosquitto_pub -h BROKER -t "tc001/notify" -m '{"text":"Your message here","color":"FF0000","speed":60,"repeat":3}'
```

**Notification Parameters:**
| Param | Type | Default | Description |
|-------|------|---------|-------------|
| `text` | string | required | Message to display (max 63 chars) |
| `color` | hex string | `"FFFF00"` | Text color (RRGGBB) |
| `duration` | int | `4` | Seconds to show static text |
| `speed` | int | `80` | Milliseconds per pixel shift (lower = faster scroll) |
| `repeat` | int | `2` | Number of full scroll loops |

**BYU Mode**: Notifications starting with `"BYU"` automatically prepend a 16x8 BYU oval logo with white Y on royal blue.

**LSU Mode**: Notifications starting with `"LSU"` automatically prepend a 16x8 tiger eye logo in purple, gold, and white.

**Windows cmd.exe example:**
```cmd
"C:\Program Files\mosquitto\mosquitto_pub.exe" -h 192.168.1.100 -t "tc001/notify" -m "{\"text\":\"Hello World\",\"color\":\"00FF00\"}"
```

> **Note**: PowerShell mangles JSON quotes. Use `cmd.exe` or a dedicated MQTT client.

### 📄 Page Control
```bash
mosquitto_pub -h BROKER -t "tc001/page" -m '{"page":"clock"}'
mosquitto_pub -h BROKER -t "tc001/page" -m '{"page":"weather"}'
```

### 💡 Brightness
```bash
mosquitto_pub -h BROKER -t "tc001/brightness" -m '{"brightness":100}'
mosquitto_pub -h BROKER -t "tc001/auto_brightness" -m '{"enabled":true}'
```

### 🎄 Christmas
```bash
# Trigger a "MERRY CHRISTMAS" scroll in alternating green/red
mosquitto_pub -h BROKER -t "tc001/christmas" -m '{"scroll":true}'
```

During Dec 20-26, the clock automatically enters Christmas mode:
- Clock digits alternate green/red with a gold colon
- Weather icon replaced by a decorated Christmas tree
- "MERRY CHRISTMAS" scrolls every 10 minutes

### ⚙️ Configuration
```bash
mosquitto_pub -h BROKER -t "tc001/config" -m '{"page_duration":15}'
mosquitto_pub -h BROKER -t "tc001/config" -m '{"rotation_enabled":false}'
mosquitto_pub -h BROKER -t "tc001/config" -m '{"weather_update_minutes":30}'
```

### 🕐 Time Sync
```bash
mosquitto_pub -h BROKER -t "tc001/time" -m '{"unix_time":1640995200}'
```

## 🏈 Sports Integration

The Python script monitors NCAA and ESPN APIs for live scores from tracked teams:

| Situation | Display | Color | Frequency |
|-----------|---------|-------|-----------|
| Game day | `BYU VS HOUSTON 7PM` | Blue | Hourly |
| Live game | `BYU 45 ARIZ 38 2nd Half` | Blue | Every 5 min + on score changes |
| Score change | Immediate update | Blue | Instant |
| BYU wins | `BYU WINS 85 71 VS ARIZONA` | Green | Every 10 min for 1 hour |
| BYU loss | `BYU LOSS 60 74 VS ARIZONA` | Red | Once (we don't dwell on it) |

### LSU Baseball

| Situation | Display | Color | Frequency |
|-----------|---------|-------|-----------|
| Game day | `LSU VS OLE MISS 7PM` | Purple | Hourly |
| Live game | `LSU 5 OLE MISS 3 T7` | Purple | Every 5 min + on score changes |
| LSU wins | `LSU WINS 5 3 VS OLE MISS` | Green | Every 10 min for 1 hour |
| LSU loss | `LSU LOSS 3 5 VS OLE MISS` | Red | Once |

Polling adapts automatically: 30 min between games, 2 min during live games.

## 🌪️ NWS Weather Alerts

Free from the National Weather Service — no API key needed. Monitors for:
- Tornado / severe thunderstorm warnings
- Winter storm / blizzard warnings
- Flash flood warnings
- Wind advisories / wind chill advisories
- Red flag warnings

Alerts scroll in color by severity: red (Extreme), orange-red (Severe), orange (Moderate).

## 🏠 Home Assistant Integration

```yaml
mqtt:
  sensor:
    - name: "TC001 Clock Status"
      state_topic: "tc001/status"

  switch:
    - name: "TC001 Auto Brightness"
      command_topic: "tc001/auto_brightness"
      payload_on: '{"enabled":true}'
      payload_off: '{"enabled":false}'

  button:
    - name: "TC001 Update Weather"
      command_topic: "tc001/weather"
      payload_press: '{}'

script:
  tc001_notification:
    alias: "Send TC001 Notification"
    sequence:
      - service: mqtt.publish
        data:
          topic: "tc001/notify"
          payload: '{"text":"{{ message }}","color":"{{ color | default("FFFF00") }}"}'
```

## 🐛 Troubleshooting

### Upload Failures
| Error | Cause | Fix |
|-------|-------|-----|
| "Packet content transfer stopped" | Baud too high or bad USB | Set Upload Speed to **115200**, try different cable |
| "The chip stopped responding" | Baud rate issue | Upload Speed → **115200** |
| No serial port | Missing driver | Install CH340 driver, verify data cable |

**Upload tip**: Hold middle button → plug USB → wait 3 sec → release → Upload.

### Sending Test Notifications via EMQX REST API

If you don't have `mosquitto_pub` installed, you can publish directly through the EMQX broker's REST API:

```bash
# 1. Get an auth token
TOKEN=$(curl -s -X POST "http://HOGWARTS.local:18083/api/v5/login" \
  -H "Content-Type: application/json" \
  -d '{"username": "admin", "password": "YOUR_DASHBOARD_PASSWORD"}' \
  | python3 -c "import sys,json; print(json.load(sys.stdin)['token'])")

# 2. Publish a test notification
curl -s -X POST "http://HOGWARTS.local:18083/api/v5/publish" \
  -H "Content-Type: application/json" \
  -H "Authorization: Bearer ${TOKEN}" \
  -d '{
    "topic": "tc001/notify",
    "payload": "{\"text\": \"BYU 78 GONZAGA 65\", \"color\": \"4073FF\", \"speed\": 60, \"repeat\": 2}",
    "qos": 0
  }'
```

**Success**: Returns `{"id":"..."}`. If you see `"no_matching_subscribers"`, the Ulanzi isn't connected — check its WiFi/MQTT.

### MQTT JSON Parse Errors
```
MQTT JSON parse error: InvalidInput
Failed payload: '{text:Hello,color:00FF00}'
```
**Cause**: Quotes stripped by PowerShell. **Fix**: Use `cmd.exe` instead, or an MQTT client app.

### Status Indicator Colors
- **Royal Blue** dot (bottom right): MQTT connected, normal day
- **Green** dot (bottom right): MQTT connected, recycle day
- **Light Blue** dot (bottom right): No MQTT time (fallback)

## 📁 Project Structure

```
TC001_Enhanced_SingleFile/
  TC001_Enhanced_SingleFile.ino  Main sketch (globals, setup, loop)
  config.h / config.h.example   User configuration (#define macros)
  hw_config.h                   Hardware constants, pins, enums, timing
  font_data.h / font_data.cpp   PROGMEM glyph arrays + icon bitmaps
  display.h / display.cpp       XY(), pset(), clearAll(), updateBrightness()
  font_render.h / font_render.cpp  drawLargeString, drawSmallString, width calculators
  graphics.h / graphics.cpp     Weather icons, Christmas tree, BYU logo, boot icon
  time_helpers.h / time_helpers.cpp  getCurrentTime, isNight, isRecycleDay, tempToColorF
  pages.h / pages.cpp           drawClock, drawWeather, drawNotify
  wifi_manager.h / wifi_manager.cpp  ensureWifi, fetchWeatherData
  mqtt_handler.h / mqtt_handler.cpp  ensureMqtt, mqttCallback, all MQTT handlers
  chime.h / chime.cpp           triggerChime, updateChime
  tasks.h / tasks.cpp           renderTask, networkTask (FreeRTOS)
```

Arduino IDE compiles all `.cpp` files in the sketch folder automatically — no build config needed.

## 🔐 MQTT TLS Setup

TLS encryption is disabled by default. To enable:

### 1. Enable in config.h
```cpp
#define MQTT_USE_TLS    1
#define MQTT_USER       "your_mqtt_username"  // optional
#define MQTT_PASS       "your_mqtt_password"  // optional
```

### 2. Choose Certificate Mode

**Option A: Insecure (self-signed certs)**
```cpp
#define MQTT_CA_CERT    ""  // empty = setInsecure(), accepts any cert
```

**Option B: CA certificate verification**
```cpp
#define MQTT_CA_CERT    "-----BEGIN CERTIFICATE-----\n" \
                        "MIID...your CA cert here...\n" \
                        "-----END CERTIFICATE-----\n"
```

### Memory Impact
| Resource | Without TLS | With TLS | Available |
|----------|------------|----------|-----------|
| Flash | ~800KB | ~880KB | 1.9MB partition |
| RAM (handshake) | — | +40-50KB temp | ~200KB free |
| RAM (steady) | — | +8-12KB | ~200KB free |

## 🔒 Security Notes

- **Never commit `config.h`** — contains WiFi password and API keys
- The Python script also contains API keys — keep it private
- **Enable MQTT TLS** for encrypted credentials over the network
- Secure your MQTT broker with authentication in production
- ESP32 only supports 2.4GHz WiFi

## 📄 License

MIT License — see [LICENSE](LICENSE) for details.

## 🙏 Acknowledgments

- **[FastLED](https://github.com/FastLED/FastLED)** — LED control library
- **[AWTRIX](https://github.com/Blueforcer/awtrix3)** — Display style inspiration
- **[Ulanzi TC001 Hardware Docs](https://github.com/rroels/ulanzi_tc001_hardware)** — Pin mappings and hardware reference
- **[OpenWeatherMap](https://openweathermap.org/)** — Weather API
- **[NWS API](https://api.weather.gov/)** — Free severe weather alerts
- **[ESPN API](https://site.api.espn.com/)** — Free sports scores
- **Claude Code** — Pair programming partner

---

**Made with ❤️ in Utah for the ESP32 and smart home community**
