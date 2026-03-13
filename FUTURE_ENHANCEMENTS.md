# Future Enhancements

## Move OpenWeather API from ESP32 to Python Publisher
**Priority:** Medium
**Reason:** Both the ESP32 and the Python publisher independently call the OpenWeather API every 30 minutes — redundant. The ESP32 uses plain HTTP (no TLS), exposing the API key in transit.

**Solution:** Have the Python publisher (which handles HTTPS natively with full cert verification) fetch weather and publish a retained JSON message to `tc001/weather`:
```json
{"temperature": 42.5, "condition": "Clouds", "icon": "02d"}
```
The ESP32 receives weather via MQTT instead of calling the API directly. Removes the API key from the device entirely.

**Scope:** Python publisher + 6 firmware files. Full implementation plan saved at `.claude/plans/glittery-giggling-beacon.md`.

**Display impact:** None — all weather icons and rendering stay unchanged.
