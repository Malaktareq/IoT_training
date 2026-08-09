# Local MQTT Broker & Bidirectional Motor / Telemetry Control

A self-hosted Mosquitto MQTT broker (Windows + WSL 2) paired with an ESP32-S3
that publishes live BME280 environmental telemetry and accepts full motor
control (power, direction, speed) over MQTT — with true hardware-state
feedback and a network failsafe that stops the motor safely on disconnect.

> **Day 14 + Day 15** deliverable — see [`docs/Day14_15_MQTT_Broker_Technical_Report.docx`](./docs/Day14_15_MQTT_Broker_Technical_Report.docx)
> for the full write-up (network diagnostics, firewall/NAT walkthrough,
> function-by-function firmware breakdown, and verification screenshots).

---

## Table of Contents

- [Overview](#overview)
- [Architecture](#architecture)
- [Hardware](#hardware)
- [Repository Structure](#repository-structure)
- [Part 1 — Broker Setup](#part-1--broker-setup)
  - [1. Install Mosquitto (WSL 2 / Ubuntu)](#1-install-mosquitto-wsl-2--ubuntu)
  - [2. Stop the auto-started background service](#2-stop-the-auto-started-background-service)
  - [3. mosquitto.conf](#3-mosquittoconf)
  - [4. Run the broker in the foreground](#4-run-the-broker-in-the-foreground)
  - [5. Open the port in Windows Firewall](#5-open-the-port-in-windows-firewall)
  - [6. Bridge the WSL 2 NAT boundary](#6-bridge-the-wsl-2-nat-boundary)
  - [7. Verify with mosquitto_pub / mosquitto_sub](#7-verify-with-mosquitto_pub--mosquitto_sub)
  - [8. Connect with MQTT Explorer](#8-connect-with-mqtt-explorer)
- [Part 3 — Firmware Behaviour & Safety Requirements](#part-3--firmware-behaviour--safety-requirements)
- [ESP32 Firmware Setup](#esp32-firmware-setup)
- [Testing the Full Loop](#testing-the-full-loop)
- [Definition of Done](#definition-of-done)
- [Troubleshooting](#troubleshooting)
- [References](#references)

---

## Overview

**Goal:** run an MQTT broker entirely on local infrastructure (no cloud
broker) and build a complete bidirectional system — control a DC motor
entirely through MQTT topics while monitoring live environment data and
live motor status coming back from the device.

**Goal split into two flows:**

| Direction | What moves |
|---|---|
| ESP32 → Broker | BME280 temperature / humidity / pressure, and the motor's *actual* state, direction, and speed |
| Broker → ESP32 | Motor ON/OFF, direction setpoint, speed setpoint (0–100%) |

All topics live under a structured, non-flat `esp32/` namespace, with the
motor's true state kept on separate topics from incoming commands (see the
project's technical report for the full topic table).

---

## Architecture

```
LOCAL WI-FI NETWORK
  ESP32-S3 Node (192.168.1.x)         MQTT Explorer (Windows Host)
        |  port 1883                         |  port 1883
        v                                     v
WINDOWS HOST MACHINE (physical LAN IP, e.g. 192.168.1.105)
  1. Windows Firewall inbound rule   -> allows TCP/1883
  2. netsh portproxy                 -> <LAN_IP>:1883 -> <WSL_IP>:1883
  3. Hyper-V vEthernet (NAT switch)  -> bridges the Wi-Fi NIC to the WSL virtual subnet
        WSL 2 UBUNTU (internal IP 172.29.x.x)
          4. mosquitto -c mosquitto.conf -v
             listens on 0.0.0.0:1883 across all internal interfaces
```

Two independent boundaries have to be opened before an external Wi-Fi
device can reach a broker running inside WSL 2:

1. **Windows Firewall** — blocks unsolicited inbound traffic on 1883 by default.
2. **WSL 2 NAT boundary** — WSL runs on its own Hyper-V-managed virtual
   subnet; traffic hitting the Windows physical IP is dropped unless it's
   explicitly forwarded with `netsh interface portproxy`.

---

## Hardware

| Component | Role |
|---|---|
| ESP32-S3-WROOM | Wi-Fi + MQTT client, runs the control firmware |
| L298N Dual H-Bridge | Drives the DC gear motor (direction + PWM speed) |
| DC Gear Motor | Actuator, driven forward/backward at 0–100% speed |
| BME280 (I2C) | Temperature / humidity / pressure sensor |
| IR Obstacle Sensor | Auxiliary digital input (breadboard) |
| Bench PSU / 12V supply | Powers the L298N / motor rail |

See `docs/circuit_diagram.png` for the full wiring diagram.

---

## Repository Structure

```
.
├── firmware/
│   ├── src/
│   │   ├── main.cpp              # setup(), loop(), MQTT callback, failsafe engine
│   │   └── L298N.cpp             # Motor driver class (forward/reverse/stop/brake)
│   ├── include/
│   │   ├── L298N.h
│   │   └── config.h              # Wi-Fi / broker credentials, pins, topics
│   └── platformio.ini
├── broker/
│   └── mosquitto.conf            # Broker config used for this project
├── docs/
│   ├── Day14_15_MQTT_Broker_Technical_Report.docx
│   ├── circuit_diagram.png
│   └── screenshots/
│       ├── mqtt_explorer_connection.jpg
│       ├── wsl_nat_diagnostics.jpg
│       └── final_verification.jpg
└── README.md
```

---

## Part 1 — Broker Setup

All commands below run **inside WSL 2 (Ubuntu)** unless marked *PowerShell*.

### 1. Install Mosquitto (WSL 2 / Ubuntu)

```bash
sudo apt update
sudo apt install mosquitto mosquitto-clients -y
```

### 2. Stop the auto-started background service

Installing via `apt` auto-enables Mosquitto as a `systemd` service, which
grabs port 1883 on boot. Running `mosquitto -c mosquitto.conf -v` manually
then fails with `Error: Address already in use`. Fix:

```bash
sudo systemctl stop mosquitto
sudo systemctl disable mosquitto

# confirm the port is free (should return no output)
ss -tuln | grep 1883
```

### 3. mosquitto.conf

```conf
# Bind port 1883 to 0.0.0.0 (all available network interfaces)
listener 1883

# Allow anonymous clients without username/password for local prototyping
allow_anonymous true
```

| Directive | Why |
|---|---|
| `listener 1883` (no IP) | Binds to `0.0.0.0`. Hardcoding a specific IP would crash the broker with *"Cannot assign requested address"* every time DHCP reassigns the host a new lease. |
| `allow_anonymous true` | Mosquitto v2.0+ blocks anonymous connections by default; this allows the ESP32 to connect without a username/password store — fine for local dev, **not for production**. |

### 4. Run the broker in the foreground

```bash
mosquitto -c mosquitto.conf -v
```

Keep this terminal open — verbose logs confirm every client connect,
subscribe, and disconnect in real time.

### 5. Open the port in Windows Firewall

Local loopback tests bypass the firewall entirely, so this step is easy to
forget. Run in **PowerShell** (not `cmd.exe` — `New-NetFirewallRule` is a
PowerShell cmdlet):

```powershell
New-NetFirewallRule -DisplayName "Mosquitto MQTT Broker" -Direction Inbound `
  -LocalPort 1883 -Protocol TCP -Action Allow
```

**Change log:** added one Inbound Rule — *Mosquitto MQTT Broker*,
Direction = Inbound, Protocol = TCP, LocalPort = 1883, Action = Allow,
Profile = Any.

### 6. Bridge the WSL 2 NAT boundary

`ping` only proves the host is alive (ICMP) — it says nothing about TCP
port 1883. WSL 2 lives on its own isolated subnet, so LAN traffic hitting
the Windows physical IP still needs to be forwarded across.

```bash
# 1. Find WSL's internal IP (run inside WSL)
hostname -I | awk '{print $1}'
# e.g. 172.29.160.2
```

```powershell
# 2. Add the port proxy (run in PowerShell as Administrator)
netsh interface portproxy add v4tov4 listenport=1883 listenaddress=<WINDOWS_LAN_IP> `
  connectport=1883 connectaddress=<YOUR_WSL_IP>

# 3. Verify TCP reachability
Test-NetConnection -ComputerName <WINDOWS_LAN_IP> -Port 1883
# Expect: TcpTestSucceeded : True
```

### 7. Verify with mosquitto_pub / mosquitto_sub

Test the broker on its own, **before** involving the ESP32 — two terminals,
same host:

```bash
# Terminal A — subscribe to everything under esp32/
mosquitto_sub -h 127.0.0.1 -p 1883 -t "esp32/#" -v
```

```bash
# Terminal B — publish a test motor command
mosquitto_pub -h 127.0.0.1 -p 1883 -t "esp32/motor/cmd/power" -m "on"
mosquitto_pub -h 127.0.0.1 -p 1883 -t "esp32/motor/cmd/direction" -m "forward"
mosquitto_pub -h 127.0.0.1 -p 1883 -t "esp32/motor/cmd/speed" -m "75"
```

### 8. Connect with MQTT Explorer

Install natively on **Windows** (not inside WSL) — it exercises the exact
same path a Wi-Fi client (the ESP32) has to take, through the firewall
rule and the portproxy bridge.

| Field | Value |
|---|---|
| Protocol | `mqtt://` |
| Host | `<WINDOWS_LAN_IP>` (e.g. `192.168.1.105`) |
| Port | `1883` |
| Username / Password | *blank* |

A successful connection shows `$SYS` topics in the tree and logs a new
client connection in the running `mosquitto -c mosquitto.conf -v` terminal.

---

## Part 3 — Firmware Behaviour & Safety Requirements

| Requirement | Implementation |
|---|---|
| Non-blocking sensor timing | `loop()` compares `millis()` against a stored `lastMsgTime`; BME280 is read/published only once the interval (2000 ms) has elapsed — no `delay()` blocking. |
| Status reflects real state, not last command | `publishMotorStatus()` reads `motor.getSpeed()` / `motor.getDirection()` **directly from the L298N driver**, never from the last received command. |
| Speed validation & clamping | Every `esp32/motor/speed` payload is parsed with `message.toInt()` and passed through `constrain(value, 0, 100)` before it is stored or applied. |
| Reconnect handling (no crash, safe stop) | `ensureConnections()` checks `WiFi.status()` / `client.connected()` every loop pass. On any disconnect it immediately forces `motor.stop()`, then retries inside a bounded, non-blocking window and auto-resubscribes once reconnected. |

### Key firmware functions

| Function | Responsibility |
|---|---|
| `setup()` | Serial, motor driver, I2C/BME280 probe (`0x76`/`0x77`), Wi-Fi, MQTT client/callback registration |
| `loop()` | Non-blocking scheduler: network servicing, timed telemetry publish, continuous motor hardware application |
| `mqttCallback()` | Parses incoming payloads, routes by topic, clamps speed, re-applies hardware + republishes status |
| `ensureConnections()` | Wi-Fi/MQTT reconnect logic with a bounded retry window and forced motor stop on disconnect |
| `applyMotorHardware()` | Single choke point for driving the motor — refuses to drive unless Wi-Fi + MQTT + power are all valid |
| `publishMotorStatus()` | Reads true hardware state from the `L298N` instance and publishes it back to MQTT |

---

## ESP32 Firmware Setup

1. Open `firmware/` in PlatformIO (VS Code) or the Arduino IDE.
2. Edit `firmware/include/config.h`:
   ```cpp
   #define WIFI_SSID     "your_wifi_ssid"
   #define WIFI_PASSWORD "your_wifi_password"
   #define MQTT_BROKER   "192.168.1.105"   // Windows LAN IP running mosquitto
   #define MQTT_PORT     1883
   ```
3. Wire the hardware per `docs/circuit_diagram.png`.
4. Build and flash:
   ```bash
   pio run -t upload
   pio device monitor
   ```
5. Confirm in the serial monitor that Wi-Fi connects and telemetry starts
   publishing.

---

## Testing the Full Loop

1. Start the broker (`mosquitto -c mosquitto.conf -v`) and leave it running.
2. Power on the ESP32 — it should connect to Wi-Fi and start publishing
   `esp32/sensor/*` topics within a few seconds.
3. Open MQTT Explorer, connect, and confirm `esp32/sensor/*` and
   `esp32/motor/*` are updating live.
4. Publish commands from MQTT Explorer's **Publish** panel:
   - `esp32/motor/power` → `on`
   - `esp32/motor/direction` → `forward`
   - `esp32/motor/speed` → `50`
5. Confirm the motor responds and `esp32/motor/state` / `speed` /
   `direction` update to match — live, not just echoing the command.
6. **Failsafe check:** stop the broker or disable Wi-Fi on the ESP32's
   access point. The motor should stop immediately. Restart the broker /
   restore Wi-Fi — the ESP32 should reconnect and resume automatically,
   with no reset required.

---

## Definition of Done

- [x] Broker runs from the terminal with a custom `mosquitto.conf`, verbose output visible
- [x] ESP32 connects over Wi-Fi to the PC's local IP and stays connected
- [x] BME280 data publishes continuously and is visible in MQTT Explorer
- [x] Publishing ON/OFF, direction, and speed commands from MQTT Explorer controls the motor correctly
- [x] Motor status and speed percentage topics update live and match the motor's real behaviour
- [x] Stopping the broker or cutting Wi-Fi stops the motor safely; the ESP32 reconnects automatically when the broker returns
- [x] Topic structure documented (this README)

---

## Troubleshooting

| Symptom | Likely Cause | Fix |
|---|---|---|
| `Error: Address already in use` on broker start | The `apt`-installed systemd `mosquitto` service is already bound to 1883 | `sudo systemctl stop mosquitto && sudo systemctl disable mosquitto` |
| `PingSucceeded: True` but `TcpTestSucceeded: False` | Firewall rule and/or WSL NAT port proxy missing | Re-check Section [5](#5-open-the-port-in-windows-firewall) and [6](#6-bridge-the-wsl-2-nat-boundary) |
| `New-NetFirewallRule` not recognized | Command run in `cmd.exe` instead of PowerShell | Run `powershell` first, then the command |
| MQTT Explorer connects locally but ESP32 never appears | Broker bound to `127.0.0.1` instead of `0.0.0.0`, or `MQTT_BROKER` in `config.h` points to the wrong IP | Confirm `listener 1883` has no IP suffix in `mosquitto.conf`; confirm `config.h` uses the Windows **LAN** IP |
| WSL IP changed after a reboot and the broker became unreachable | WSL 2's internal IP is not static by default | Re-run `hostname -I` and re-apply the `netsh interface portproxy` rule with the new address |
| Motor doesn't stop when Wi-Fi drops | Custom firmware modification bypassing `applyMotorHardware()` | All motor writes must go through `applyMotorHardware()` — it is the single failsafe choke point |

---

## References

- [Mosquitto MQTT Broker: Pros, Cons, Tutorial and Modern Alternatives — EMQX](https://www.emqx.com/en/blog/mosquitto-mqtt-broker-pros-cons-tutorial-and-modern-alternatives)
- [MQTT Explorer](https://mqtt-explorer.com)
- [Eclipse Mosquitto documentation](https://mosquitto.org/documentation/)
