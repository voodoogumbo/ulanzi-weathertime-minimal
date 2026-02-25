# TC001 Enhanced Clock — TODO

## Unused Hardware (Available on Board)
- [ ] **Auto-Brightness via Light Sensor** — GL5516 photoresistor on GPIO35, currently unused. Could replace or supplement the time-based day/night brightness switching.
- [ ] **Button Controls** — Left (GPIO26), Middle (GPIO27), Right (GPIO14) are wired but unused. Ideas: manual page switching, brightness override, dismiss notifications, toggle chime.
- [ ] **Indoor Temperature** — SHT3x sensor on I2C (0x44) is onboard. Could show indoor vs outdoor temp, or add a dedicated indoor temp page.
- [ ] **RTC Backup** — DS1307 RTC on I2C (0x68) could provide offline time when MQTT is unavailable, replacing the "TIME?" fallback.

## Display Enhancements
- [ ] **Mist/Haze/Fog Icon** — Weather conditions like Mist, Haze, Smoke, Fog currently fall through to the default cloud icon. A dedicated hazy/foggy icon would be more accurate.
- [ ] **AM/PM Indicator** — Optional small AM/PM text next to the clock for 12-hour preference.
- [ ] **Date Display Page** — Add a page showing month/day (e.g., "FEB 25") in the page rotation.
- [ ] **Sunrise/Sunset Page** — OpenWeather API already returns sunrise/sunset timestamps.

## MQTT & Connectivity
- [ ] **MQTT Authentication** — Support username/password auth for secured brokers.
- [ ] **Configurable Timezone via MQTT** — Currently hardcoded to Mountain Time. An MQTT topic could set the TZ string at runtime.
- [ ] **Status Heartbeat** — Periodically publish uptime, WiFi RSSI, free heap to a `tc001/status` topic for monitoring.
- [ ] **Brightness MQTT Control** — The README documents `tc001/brightness` and `tc001/auto_brightness` topics but the firmware doesn't subscribe to them yet.

## Quality of Life
- [ ] **Chime Volume/Disable** — MQTT command to enable/disable the hourly chime or adjust duration.
- [ ] **Notification Queue** — Currently a new notification overwrites the active one. A queue would let multiple alerts display in sequence.
- [ ] **Persistent Config** — Save page_duration, rotation_enabled, and other runtime config to SPIFFS/NVS so they survive reboots.
- [ ] **Boot Animation** — Replace the static skull icon with a short animation sequence.
