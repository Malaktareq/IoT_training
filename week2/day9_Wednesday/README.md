# ESP32 BME280 Environmental Monitor (Access Point Dashboard)

A self-contained environmental monitoring station built on an **ESP32** and a **BME280** sensor. The ESP32 broadcasts its own Wi-Fi network (Access Point mode), serves a live sensor dashboard, and logs readings to a downloadable CSV file — no router or internet connection required.

## Features

- 📡 Reads **temperature**, **humidity**, and **pressure** from a BME280 sensor over I2C
- 🌐 ESP32 runs as a Wi-Fi **Access Point** — connect directly with a laptop or phone, anywhere
- 📊 Live dashboard at `http://192.168.4.1` with auto-refreshing sensor readings
- 🔁 JSON API endpoint for programmatic access to live data
- 💾 Readings logged to a CSV file on flash storage (SPIFFS/LittleFS), downloadable from the dashboard
- ⏱️ Fully **non-blocking** — sensor polling, CSV logging, and the HTTP server all run concurrently using `millis()` instead of `delay()`

## Hardware

| Component | Notes |
|---|---|
| ESP32 dev board | Any standard ESP32 board |
| BME280 breakout | 4-pin I2C module (VIN, GND, SDA, SCL) |
| Jumper wires | For I2C wiring |

### Wiring

| BME280 Pin | ESP32 Pin |
|---|---|
| VIN | 3.3V |
| GND | GND |
| SDA | GPIO 21 |
| SCL | GPIO 22 |

> ℹ️ On 4-pin BME280 breakouts, the I2C address (`0x76` or `0x77`) is fixed internally by the manufacturer. Run the included I2C scanner sketch once to confirm which address your module answers to.

## Getting Started

### 1. Install dependencies

**Arduino IDE**
```
Adafruit Unified Sensor
Adafruit BME280 Library
```

**PlatformIO** (`platformio.ini`)
```ini
lib_deps =
    adafruit/Adafruit Unified Sensor
    adafruit/Adafruit BME280 Library
```

### 2. Configure the Access Point

Set your network name and password in the firmware:
```cpp
const char* AP_SSID     = "YOUR_AP_SSID";
const char* AP_PASSWORD = "YOUR_AP_PASSWORD";   // minimum 8 characters
```

### 3. Set the BME280 I2C address

If the I2C scanner reports a different address than `0x76`, update it:
```cpp
#define BME280_I2C_ADDRESS 0x76
```

### 4. Upload the filesystem and firmware

Upload the `data/` folder (contains `index.html`) to SPIFFS/LittleFS, then flash the sketch to the ESP32.

### 5. Connect and view the dashboard

1. Power on the ESP32
2. On your laptop or phone, join the **YOUR_AP_SSID** Wi-Fi network
3. Open a browser to `http://192.168.4.1`

## API

| Route | Method | Description |
|---|---|---|
| `/` | GET | Live HTML dashboard |
| `/data` | GET | Latest sensor reading as JSON |
| `/download` | GET | Download the accumulated CSV log |

**Example `/data` response:**
```json
{ "temperature": 24.50, "humidity": 48.20, "pressure": 1013.25 }
```

## How it works

- **Sensor polling & logging** — the BME280 is read and a new CSV row is appended roughly every 10 seconds, timed with `millis()` so the loop never blocks.
- **Storage** — sensor history is written to `/data.csv` on SPIFFS/LittleFS (files), while Wi-Fi credentials and small settings would use NVS/Preferences (key-value) if added later.
- **Networking** — the ESP32 runs in `WIFI_AP` mode via `WiFi.softAP()`, acting as its own router and DHCP server at the fixed address `192.168.4.1`.

## Project Structure

```
.
├── src/
│   └── main.cpp          # Sensor reading, Wi-Fi AP, HTTP routes
├── data/
│   └── index.html         # Dashboard served from flash storage
├── platformio.ini
└── README.md
```

## Troubleshooting

- **BME280 not found** — double-check wiring and confirm the I2C address with a scanner sketch (`Wire.begin(21, 22)` + address sweep 1–127).
- **Can't see the YOUR_AP_SSID network** — make sure `WiFi.mode(WIFI_AP)` runs before `WiFi.softAP()`, and that the password is at least 8 characters.
- **Dashboard doesn't load** — confirm `index.html` was uploaded to SPIFFS/LittleFS, not just compiled into the sketch.

## License

MIT — feel free to use and modify for your own projects.
