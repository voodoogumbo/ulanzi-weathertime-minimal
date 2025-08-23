# TC001 Enhanced Clock 🕐

A complete, feature-rich smart clock firmware for the TC001 32x8 LED matrix device with AWTRIX-style display, weather integration, and robust MQTT control.

## 🚀 **Latest Enhancements**
- 🐕 **LaMetric Watchdog Boot Icon**: Professional startup icon in LaMetric orange
- 🌡️ **Dynamic Temperature Colors**: Gradient scaling from white (0°F) to red (100°F+)
- 🌙 **Ultra-Dim Night Mode**: Brightness drops to 1 after 10pm for bedroom use
- ⚡ **Performance Optimized**: NTP removed, MQTT-only time sync for faster operation
- 🛡️ **Rock-Solid Stability**: 30-second watchdog timer prevents reboots
- 🕐 **MQTT Time Sync**: Unix timestamp-based time with automatic Mountain Time/DST
- ☁️ **3D Cloud Gradients**: Sophisticated 5-row cloud shading with realistic depth
- 🔍 **Time Source Indicator**: Green pixel = MQTT time active
- 📦 **Condensed Code**: 192 lines removed, optimized for maintainability

[![Arduino](https://img.shields.io/badge/Arduino-ESP32-blue.svg)](https://www.arduino.cc/)
[![License](https://img.shields.io/badge/License-MIT-green.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-ESP32-red.svg)](https://espressif.com/en/products/socs/esp32)

## ✨ Features

### 🔥 **Core Display Features**
- **32x8 LED Matrix**: Full-width AWTRIX TMODE5-style clock display
- **Optimized Time Display**: Perfect pixel spacing, no text overflow
- **Enhanced Colon**: Two separate 2x2 blocks with clean blinking animation
- **Page Rotation**: Clock → Weather (configurable timing)
- **Ultra-Dim Night Mode**: Automatic red display + brightness = 1 after 22:00 (10pm)
- **LaMetric Watchdog Boot**: Professional startup icon in signature orange

### 🌤️ **Smart Weather Integration**
- **OpenWeather API**: Real-time weather data with 15-minute updates
- **Day/Night Icons**: Intelligent sun/moon switching based on API data
- **3D Cloud Gradients**: Revolutionary 5-row gradient system with realistic atmospheric depth
  - **Row 5 (bottom)**: Dark gray shadow base
  - **Row 4**: Medium gray with dark right edge (shadow transition)
  - **Row 3**: Light gray with medium right edge (highlight transition)
  - **Rows 2 & 1**: Bright highlight (sunlit cloud top)
- **Temperature Color Scaling**: Dynamic color gradient from white (0°F) to red (100°F+)
- **Weather Icon Colors**: Unique colors for each weather condition
  - ☀️ **Sun**: Golden yellow - ☁️ **3D Clouds**: Multi-gray gradient
  - 🌙 **Moon**: Silver-blue - 🌧️ **Rain**: Deep blue 
  - ❄️ **Snow**: Pure white - ⛈️ **Thunderstorm**: Electric purple

### 📡 **Advanced MQTT Control**
- **MQTT Time Sync**: Unix timestamp-based time with automatic Mountain Time/DST conversion
- **Performance Optimized**: NTP removed for faster, more reliable operation
- **Lightning-Fast Response**: Instant MQTT processing with consolidated topic routing
- **Visual Time Indicator**: Green pixel shows active MQTT time source
- **Comprehensive Topics**: Full device control via MQTT (`/time`, `/notify`, `/weather`, etc.)
- **Enhanced Debugging**: Detailed logging with hex dump analysis

### 🔧 **Robust Architecture**
- **Dual-Core FreeRTOS**: Dedicated cores for display and networking
- **Ultra-Stable Watchdog**: 30-second timeout for maximum stability
- **Network Stability**: Bulletproof WiFi/MQTT reconnection logic
- **Mountain Time**: Automatic DST handling with timezone accuracy
- **Optimized Code**: 1161 lines (192 lines removed), cleaner and more maintainable

## 🛠️ Hardware Requirements

- **TC001 Device** (32x8 WS2812B LED matrix)
- **ESP32** microcontroller
- **Light sensor** on GPIO35 (for auto-brightness)
- **WiFi connection**
- **MQTT broker** (Home Assistant, Mosquitto, etc.)

## 🚀 Quick Setup

### 1. **Download & Configure**
```bash
git clone https://github.com/yourusername/tc001-enhanced-clock.git
cd tc001-enhanced-clock
cp config.h.example config.h
```

### 2. **Edit Configuration**
Open `config.h` and fill in your settings:
```cpp
const char* WIFI_SSID     = "YourWiFiNetwork";
const char* WIFI_PASSWORD = "YourWiFiPassword";
const char* MQTT_HOST     = "192.168.1.100";  // Your MQTT broker
const char* OPENWEATHER_API_KEY = "your_api_key_here";
const char* OPENWEATHER_LAT = "40.7128";      // Your latitude
const char* OPENWEATHER_LON = "-74.0060";     // Your longitude
```

### 3. **Setup MQTT Time Source**
**IMPORTANT**: This clock requires MQTT time sync (NTP removed for performance)
1. Install Python requirements: `pip install paho-mqtt`
2. Edit `mqtt_time_publisher.py` with your MQTT broker settings
3. Run continuously: `python3 mqtt_time_publisher.py`

### 4. **Install Dependencies**
In Arduino IDE, install these libraries:
- `FastLED` by Daniel Garcia
- `PubSubClient` by Nick O'Leary  
- `ArduinoJson` by Benoit Blanchon

### 5. **Upload Firmware**
1. Open `TC001_Enhanced_SingleFile.ino` in Arduino IDE
2. Select **ESP32 Dev Module** board
3. Upload to your TC001 device
4. Watch for LaMetric watchdog boot icon and green time indicator!

## 📋 API Setup Guides

### OpenWeather API
1. Sign up at [OpenWeatherMap](https://openweathermap.org/api)
2. Get your free API key
3. Find your coordinates at [LatLong.net](https://www.latlong.net/)

### MQTT Broker Options
- **Home Assistant**: Built-in MQTT broker
- **Mosquitto**: Standalone MQTT broker
- **Cloud**: AWS IoT, HiveMQ Cloud, etc.

## 📡 MQTT Topics & Commands

Replace `tc001` with your configured `MQTT_BASE` value.

### 📢 **Notifications**
```bash
# Show notification with custom color and duration
mosquitto_pub -h YOUR_BROKER -t "tc001/notify" -m '{"text":"Hello World","color":"FF0000","duration":5}'

# Simple text notification (default yellow, 4 seconds)
mosquitto_pub -h YOUR_BROKER -t "tc001/notify" -m '{"text":"Meeting in 5 min"}'
```

### 📄 **Page Control**
```bash
# Switch to specific page
mosquitto_pub -h YOUR_BROKER -t "tc001/page" -m '{"page":"clock"}'
mosquitto_pub -h YOUR_BROKER -t "tc001/page" -m '{"page":"calendar"}'
mosquitto_pub -h YOUR_BROKER -t "tc001/page" -m '{"page":"weather"}'
```

### 💡 **Brightness Control**
```bash
# Set manual brightness (1-255)
mosquitto_pub -h YOUR_BROKER -t "tc001/brightness" -m '{"brightness":100}'

# Toggle auto-brightness
mosquitto_pub -h YOUR_BROKER -t "tc001/auto_brightness" -m '{"enabled":true}'
mosquitto_pub -h YOUR_BROKER -t "tc001/auto_brightness" -m '{"enabled":false}'
```

### ⚙️ **Configuration**
```bash
# Change page rotation timing (seconds)
mosquitto_pub -h YOUR_BROKER -t "tc001/config" -m '{"page_duration":15}'

# Disable page rotation
mosquitto_pub -h YOUR_BROKER -t "tc001/config" -m '{"rotation_enabled":false}'

# Change weather update interval (minutes)
mosquitto_pub -h YOUR_BROKER -t "tc001/config" -m '{"weather_update_minutes":30}'
```

### 🌤️ **Weather Control**
```bash
# Force immediate weather update
mosquitto_pub -h YOUR_BROKER -t "tc001/weather" -m '{}'
```

### 🕐 **Time Synchronization**
```bash
# Send Unix timestamp for time sync (replaces NTP)
mosquitto_pub -h YOUR_BROKER -t "tc001/time" -m '{"unix_time":1640995200}'

# Or use the included Python script for continuous time publishing
python3 mqtt_time_publisher.py
```

**Python Time Publisher Setup:**
1. Install requirements: `pip install paho-mqtt`
2. Edit `mqtt_time_publisher.py` to match your MQTT broker settings
3. Run continuously: `python3 mqtt_time_publisher.py`
4. The script publishes Unix timestamps every 60 seconds
5. Clock will show green pixel (bottom right) when using MQTT time vs blue for NTP

## 🏠 Home Assistant Integration

Add to your `configuration.yaml`:

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
          payload: '{"text":"{{ message }}","color":"{{ color | default(\"FFFF00\") }}","duration":{{ duration | default(4) }}}'
```

## 🐛 Troubleshooting

### **MQTT JSON Parse Errors**
The firmware uses fast, strict JSON parsing for maximum responsiveness:
```
❌ MQTT JSON parse error: InvalidInput
📝 Failed payload: '{"text":"Test from PC"}'
```

**Common Fixes:**
- Ensure proper JSON formatting with quoted keys: `{"text":"message"}`
- Check for hidden characters (use hex dump in logs)
- Verify MQTT broker settings
- Test with simple payloads first: `{"text":"hello"}`

### **Weather Not Updating**
1. Verify API key is valid and active
2. Check coordinates are correct (decimal degrees)
3. Monitor logs for HTTP response codes
4. Ensure internet connectivity

### **WiFi Connection Issues**
1. Check SSID and password in `config.h`
2. Verify network is 2.4GHz (ESP32 limitation)
3. Monitor watchdog logs for network timeouts

### **Display Issues**
1. Verify LED_PIN is correct (GPIO32 for TC001)
2. Check power supply capacity (32x8 LEDs = ~15W max)
3. Adjust brightness for power limitations

## 🔒 Security Notes

- **Never commit `config.h`** to public repositories
- Use strong WiFi passwords
- Secure your MQTT broker with authentication
- Consider VPN for remote MQTT access

## 🤝 Contributing

1. Fork the repository
2. Create your feature branch (`git checkout -b feature/amazing-feature`)
3. Commit your changes (`git commit -m 'Add amazing feature'`)
4. Push to the branch (`git push origin feature/amazing-feature`)
5. Open a Pull Request

## 📄 License

This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- **FastLED Library** - Incredible LED control library
- **AWTRIX** - Inspiration for the display style
- **OpenWeatherMap** - Free weather API
- **Home Assistant Community** - MQTT integration patterns

---

**Made with ❤️ for the ESP32 and smart home community**

For questions and support, please open an issue on GitHub.