# Task 3.3 — Firebase Cloud Logging (Continuation of Task 3.1)

Dual-tier IoT telemetry system: an ESP32-S3 (BME280 sensor + L298N-driven DC motor) publishes over MQTT to a self-hosted Mosquitto broker in WSL. Node-RED consumes that telemetry, drives a local live dashboard (Task 3.1), and — as of this task — mirrors the same validated data into a **Firebase Realtime Database**, giving the system a cloud record on top of the local dashboard.

> Full write-up: [`reports/Task_3_3_Firebase_Cloud_Logging_Report.docx`](reports/Task_3_3_Firebase_Cloud_Logging_Report.docx) / `.pdf`. Day-by-day working notes: [`reports/Day_21_report.docx`](reports/Day_21_report.docx) / `.pdf`. Original assignment brief: `reports/Task_3.3.docx`.

## Goal

Extend the Node-RED system from Task 3.1 so live sensor data and motor state also land in Firebase — without duplicating the validation/clamping logic that already exists, and without slowing down the local dashboard.

## Repository Structure

```
DAY21_SUBMISSION/
├── json files/
│   ├── flows.json                 # Exported Node-RED flow (Task 3.1 + Task 3.3 Firebase branch)
│   └── iot-training-92983-...json # Firebase Realtime Database export (live/history snapshot)
│
├── local_broker/                  # PlatformIO project — ESP32-S3 firmware
│   ├── include/
│   │   ├── config.h                # WiFi / MQTT broker / topic definitions
│   │   ├── L298N.h                 # Motor driver abstraction (direction, speed, stop)
│   │   ├── project.h               # Shared project-wide declarations
│   │   └── README                  # Notes on the include layout
│   ├── lib/                        # PlatformIO library dependencies
│   ├── src/                        # Firmware entry point (setup/loop, MQTT callback, sensor loop)
│   ├── test/                       # PlatformIO unit tests
│   ├── .pio/ .vscode/               # Build & editor artifacts (not tracked for review)
│   ├── platformio.ini              # Board, framework and library configuration
│   ├── .gitignore
│   └── README.md                   # Firmware-specific setup notes
│
├── media/                          # Screenshots / screen recordings referenced by the reports
│
├── reports/
│   ├── Day_21_report.docx / .pdf              # Raw daily working notes (Notion export)
│   ├── Task_3_3_Firebase_Cloud_Logging_Report.docx / .pdf   # Full engineering report (this task)
│   └── Task_3.3.docx                          # Original assignment brief
│
└── README.md                       # This file
```

## Part 1: Firebase Setup

- **Project:** Firebase project created under the Google Cloud ecosystem.
- **Database:** Realtime Database instance provisioned in the `europe-west1` (Belgium) region.
  Endpoint: `https://iot-training-92983-default-rtdb.europe-west1.firebasedatabase.app/`
- **Security Rules (Test Mode):**

  ```json
  {
    "rules": {
      ".read": true,
      ".write": true
    }
  }
  ```

  Test-mode rules were used deliberately for this training exercise so the Node-RED `http request` node could write over plain HTTPS with no service-account key. **Rules must be tightened before any long-running or public deployment.**
- **Node choice:** the dedicated `node-red-contrib-firebase-realtime-database` palette was evaluated and **bypassed** in favour of the core, dependency-free `http request` node (standard `PUT`/`POST` verbs, no service-account JSON to manage). See the full comparison table in the engineering report, Section 4.

## Part 2: How the Firebase Branch Was Added — Without Duplicating Logic

Task 3.1's flow already receives BME280 telemetry and true motor state over MQTT and validates/clamps it in `function 1` / `function 2` before driving the dashboard widgets. **The Firebase branch taps directly off those existing validation nodes** rather than building a second, parallel ingestion path — so the data written to the cloud is guaranteed to be identical to what's shown on the local gauges, and no parsing/clamping code exists twice.

```
ESP32 → Mosquitto (WSL) → Node-RED validation/clamping (Task 3.1)
                                   │
                    ┌──────────────┴───────────────┐
                    ▼                               ▼
           UI Dashboard (local,              Firebase branch (async,
           zero-latency)                     never blocks the UI)
```

Because Node-RED's event loop dispatches downstream wires concurrently, the cloud `HTTP PUT`/`POST` calls run entirely in the background — internet latency, TLS handshakes and Firebase response time never delay the local dashboard render cycle.

### Data Structure: Current State vs. History Log

The database is split into two purpose-built namespaces:

| Path | Written by | HTTP verb | Behaviour | Purpose |
|---|---|---|---|---|
| `/live` | State Aggregator (`function 4`) | `PUT` | Overwrite — idempotent, always reflects "right now" | Instantaneous mirror of the local dashboard state |
| `/history` | Prepare History Snapshot (15 s Inject) | `POST` | Append — auto-generated, chronologically sorted push key per entry | Immutable time-series log for trend analysis |

```json
{
  "live": {
    "temperature": 24.8, "humidity": 45.2, "pressure": 1013.25,
    "motor_running": true, "direction": "CW", "speed": 180,
    "timestamp": 1786873200000
  },
  "history": {
    "-O4jK9L1mNOpQ2R3sT4": {
      "temperature": 24.8, "humidity": 45.2, "pressure": 1013.25,
      "motor_running": true, "direction": "CW", "speed": 180,
      "timestamp": 1786873200000
    }
  }
}
```

**Why two verbs:** `PUT` targets an exact path and is idempotent, which is what a live "current state" needs. `POST` lets Firebase generate a unique key per call, which is what an append-only, unbounded history log needs — every 15-second interval creates a new, immutable record instead of overwriting the last one.

**Live state (`/live`):** a `function 4` state-aggregator node holds an in-memory `flow.get('current_state')`/`flow.set(...)` snapshot. Because the six sensor/motor MQTT topics arrive independently and asynchronously, writing straight from each `MQTT IN` node to Firebase with `PUT` would overwrite the whole `/live` object on every message. The aggregator instead merges each incoming topic into the cached snapshot and re-`PUT`s the full, current document every time — so no field is ever lost.

**History log (`/history`):** a 15-second `Inject` node reads the same cached `flow.get('current_state')` snapshot (already validated by Task 3.1 and already merged by `function 4`) and `POST`s a timestamped copy to `/history.json`. No sensor data is read a second time from MQTT — it reuses the aggregator's output, which is the "zero logic duplication" guarantee.

### Topic Reference (unchanged from Task 3.1, reused as the Firebase source)

| Topic | Payload | Consumed by |
|---|---|---|
| `esp32/sensor/temperature` | float, °C | `function 4` → `/live.temperature` |
| `esp32/sensor/humidity` | float, %RH | `function 4` → `/live.humidity` |
| `esp32/sensor/pressure` | float, hPa | `function 4` → `/live.pressure` |
| `esp32/motor/power` | `running` \| `stopped` | `function 4` → `/live.motor_running` |
| `esp32/motor/direction` | `forward` \| `backward` \| `stopped` | `function 4` → `/live.direction` |
| `esp32/motor/speed` | integer 0–100 | `function 4` → `/live.speed` |
| `esp32/status` | `online` \| `offline` (retained + LWT) | Reused for offline detection — see below |

### LWT / Offline Detection

Task 3.1's MQTT Last Will and Testament on `esp32/status` (registered at `client.connect()`, QoS 1, retained, will message `"offline"`) is the same signal the Task 3.1 dashboard uses for its `ONLINE`/`OFFLINE` indicator. Wiring `esp32/status` into a dedicated `PUT .../system.json` branch — so an unexpected ESP32 disconnect also shows up in Firebase instead of the cloud silently keeping stale "online" data — is the one item still open; see **Remaining Work** below and Section 12 of the full engineering report.

## Verification

1. Isolated `Inject → http request (PUT) → Debug` test chain confirmed a single write to `/test.json` before any live sensor data was routed (Firebase Console flashed the key green on trigger).
2. `/live` was confirmed to update in the Firebase Console in real time, matching the Task 3.1 dashboard's gauges and indicators.
3. `/history` was confirmed to accumulate a new, uniquely-keyed entry every 15 seconds without disturbing `/live`.
4. Firebase writes were confirmed **not** to introduce any visible lag on the local dashboard, since they run on a separate downstream branch off the validation nodes.

Two hardware/logic bugs were diagnosed and fixed during this task — a `NaN` BME280 reading traced to an unaddressed I2C bus, and a zeroed `/live` payload traced to the aggregator being wired to dashboard *widget* outputs instead of the upstream MQTT/validation nodes (dashboard nodes strip `msg.topic`, so the aggregator's topic-matching silently failed). Full diagnostic log in the engineering report, Sections 6.3 and 8.4.

## Definition of Done

- [x] Firebase Realtime Database created and receiving writes from Node-RED
- [x] Current sensor + motor state visible live in the Firebase console, matching the Node-RED dashboard in real time
- [x] Historical entries accumulate over time with timestamps, at a documented 15-second interval
- [x] Updated Node-RED flow exported (`flows.json`) included in the repo, alongside the Task 3.1 version
- [x] README explains the data structure (current vs. history) and how the Firebase branch was added without duplicating logic (this document)

