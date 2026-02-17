# TC001 Enhanced Clock 🕐

A feature-rich smart clock firmware for the Ulanzi TC001 32x8 LED matrix with AWTRIX-style display, scrolling notifications, live BYU sports scores, NWS weather alerts, and MQTT control.

## 🚀 What's New (Feb 2026)

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

### 🔧 Architecture
- **Dual-Core FreeRTOS**: Core 1 renders, Core 0 handles networking (no scroll stutter)
- **30-Second Watchdog**: Prevents hangs with task-level monitoring
- **Adaptive FPS**: 10 FPS for static display, 20 FPS during scroll animations
- **Task Resurrection**: Main loop auto-restarts crashed render/network tasks
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
const char* WIFI_SSID     = "YourWiFiNetwork";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const char* MQTT_HOST     = "192.168.1.100";
const char* OPENWEATHER_API_KEY = "your_api_key_here";
const char* OPENWEATHER_LAT = "XX.XXXX";
const char* OPENWEATHER_LON = "-XXX.XXXX";
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
Partition Scheme: Default 4MB with spiffs (1.2MB APP/1.5MB SPIFFS)
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

Edit the config section in `mqtt_time_recycling_publisher.py`:
```python
MQTT_HOST = "192.168.1.100"       # Your MQTT broker
OPENWEATHER_API_KEY = "your_key"  # For freeze warnings
OPENWEATHER_LAT = "XX.XXXX"       # Your location
OPENWEATHER_LON = "-XXX.XXXX"
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

## 🏈 BYU Sports Integration

The Python script monitors ESPN's free API for BYU Cougars men's basketball and football:

| Situation | Display | Color | Frequency |
|-----------|---------|-------|-----------|
| Game day | `BYU VS HOUSTON 7PM` | Blue | Hourly |
| Live game | `BYU 45 ARIZ 38 2nd Half` | Blue | Every 5 min + on score changes |
| Score change | Immediate update | Blue | Instant |
| BYU wins | `BYU WINS 85 71 VS ARIZONA` | Green | Every 10 min for 1 hour |
| BYU loss | `BYU LOSS 60 74 VS ARIZONA` | Red | Once (we don't dwell on it) |

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

## 🔒 Security Notes

- **Never commit `config.h`** — contains WiFi password and API keys
- The Python script also contains API keys — keep it private
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
