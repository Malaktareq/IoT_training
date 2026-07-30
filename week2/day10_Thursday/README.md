# ESP-NOW Two-Way Wireless Interlock System

A peer-to-peer wireless safety interlock built on two ESP32 boards using **ESP-NOW** — no Wi-Fi router, access point, or internet connection required. Each board reads its own local IR sensor and uses that reading to control the *other* board's motor, while simultaneously receiving commands sent back the other way. A 2-second fail-safe timeout guarantees that any communication loss resolves to the safest possible state: **motor stopped**.

> Project 4 — IoT / Embedded Systems Training Program

## Overview

Neither board acts as a server, client, hub, or router. Both are simultaneous senders and receivers (transceivers) in a symmetric link:

```
[Board A] --- IR Sensor A ===[ ESP-NOW Wireless ]===> Motor B [Board B]
[Board A] <--- Motor A    <===[ ESP-NOW Wireless ]=== IR Sensor B [Board B]
```

- Board A's IR sensor input becomes Board B's motor command, and vice versa.
- If either board loses the wireless link for more than 2 seconds, it automatically stops its local motor.

## Features

- **Connectionless, low-latency comms** via ESP-NOW (MAC-address addressing, no router/AP/internet dependency)
- **Shared packet structure** (`Message`) sent identically by both boards
- **Non-blocking callbacks** (`OnDataSent`, `OnDataRecv`) — no `delay()` in the hot path
- **2-second fail-safe timeout** using `millis()`, not `delay()`
- **Heartbeat-style transmission** so "alive" vs. "disconnected" can be distinguished
- **Modular OOP design** — communication, motor control, and messaging are separated into reusable classes

## Hardware

| Component | Notes |
|---|---|
| ESP32 (x1) | Board A — MAC `78:42:1C:6C:13:68` |
| ESP32-S3 (x1) | Board B — MAC `9C:13:9E:A8:6C:B4` |
| L298N motor driver (x2) | One per board |
| DC motor (x2) | One per board |
| IR obstacle sensor (x2) | One per board |
| 12V power supply (x2) | Shared ground with each ESP32 |
| Status LEDs | RUN / STOP / fail-safe indicators |

### Wiring summary

```
ESP32 (Board)        L298N Driver         DC Motor
  [PWM GPIO] --------------> ENA
  [GPIO]     --------------> IN1
  [GPIO]     --------------> IN2
  [GND]      --------------> GND  <---- shared with 12V motor supply GND
                     OUT1/OUT2 -----------------> Motor terminals

ESP32 (Board)        IR Sensor
  [Digital IN] <------------ OUT (obstacle detected)
  [3.3V/5V]    ------------> VCC
  [GND]        ------------> GND
```

A full labeled schematic and bench photos are included in [`/docs`](./docs) (or the wiring diagram attached to this repository).

**Pre-power checklist**
- Common ground verified between each ESP32 and its own L298N (~0 Ω)
- ENA jumper physically removed before attaching the PWM line
- IR sensor GND shares the same reference as the ESP32 and motor driver
- Both boards' MAC addresses double-checked against the values hardcoded in `config.h`

## How ESP-NOW works

ESP-NOW runs on the same 2.4 GHz radio as Wi-Fi but skips the Wi-Fi/TCP-IP stack entirely — no router, no IP address, just direct MAC-to-MAC packets.

| Feature | ESP-NOW | Traditional Wi-Fi |
|---|---|---|
| Router required | No | Yes |
| Addressing | MAC address | IP address |
| Latency | Very low | Higher |
| Protocol stack | Lightweight, direct | Full TCP/IP |
| Power consumption | Low | Higher |

**Required init order on every boot:**

```
WiFi.mode(WIFI_STA)  →  esp_now_init()  →  esp_now_add_peer()  →  register callbacks  →  send/receive
```

### Shared message struct

Both boards must use byte-identical struct definitions (from a shared `Message.h`), since ESP-NOW transmits raw bytes with no serialization:

```cpp
typedef struct
{
    uint8_t  senderID;   // which board sent this message
    bool     runMotor;   // true = run, false = stop
    uint32_t timestamp;  // millis() at time of send
} Message;
```

### Fail-safe logic

```cpp
const unsigned long TIMEOUT = 2000; // 2 seconds

if (millis() - lastReceivedTime > TIMEOUT) {
    motor.stop();               // no packet in > 2s -> assume link/partner is down
} else {
    motor.setState(lastMsg.runMotor);
}
```

`millis()` is used instead of `delay()` so sensor reads, sends, and callbacks keep running uninterrupted while the timeout is monitored in parallel.

## Repository structure

```
TASK1_DAY10/
├── include/
│   ├── config.h        # Pins, peer MACs, PWM & timeout constants
│   ├── ESPNow.h         # ESP-NOW class declaration
│   ├── L298N.h          # Motor driver class declaration
│   └── Message.h        # Shared packet struct
├── lib/                 # Third-party / custom libraries
├── src/
│   ├── ESPNow.cpp       # ESP-NOW class implementation
│   ├── L298N.cpp        # Motor driver implementation
│   └── main.cpp         # setup()/loop(): read IR, send state, apply received state
├── test/
│   ├── README
│   └── recv_send_test_one_...  # Bench test for two-way packet exchange
└── .gitignore
```

## ESPNow class

The raw ESP-NOW C API is wrapped in a reusable class so `main.cpp` stays focused on interlock logic (read sensor → decide → send → apply received state):

```cpp
class ESPNow
{
public:
    ESPNow();
    bool begin();
    bool addPeer(const uint8_t *peerAddress);
    bool send(const uint8_t *peerAddress, const uint8_t *data, size_t size);
    void onReceive(void (*callback)(const uint8_t *, const uint8_t *, int));
    void onSend(void (*callback)(const uint8_t *, esp_now_send_status_t));
};
```

| Method | Responsibility |
|---|---|
| `begin()` | Set Wi-Fi station mode and initialize ESP-NOW |
| `addPeer()` | Register the partner board's MAC address as a trusted peer |
| `send()` | Transmit a raw byte buffer (the `Message` struct) to a peer |
| `onReceive()` | Register the application's receive callback |
| `onSend()` | Register the application's send/delivery-status callback |

## Getting started

1. **Get both boards' MAC addresses** via `WiFi.macAddress()` and hardcode them into `config.h`.
2. **Flash identical firmware** (with each board's own `MY_ID`) to both boards using PlatformIO.
3. **Wire each board** to its own L298N + motor and IR sensor per the diagram above.
4. **Power both boards** and confirm pairing (no external router needed).
5. **Test the interlock**: blocking Board A's IR sensor should stop Board B's motor, and vice versa.
6. **Test the fail-safe**: power off one board and confirm the other stops its motor within 2 seconds.

## Definition of done

- [x] Both ESP32s pair successfully via ESP-NOW without an external Wi-Fi router
- [x] Blocking Board A's IR sensor immediately stops Board B's motor, and vice versa
- [x] Disconnecting power from one ESP32 triggers the 2-second fail-safe on the other
- [x] Dual-MCU wiring diagram included in the repository
- [x] GitHub repository link + wiring schematic submitted

## Key takeaways

- **ESP-NOW** enables direct ESP32-to-ESP32 communication with no router, AP, or internet — addressed by MAC, not IP.
- **Callbacks** (`OnDataSent`/`OnDataRecv`) are invoked automatically by the ESP-NOW stack and must stay fast and non-blocking.
- **Shared structs** must be byte-for-byte identical on both ends since ESP-NOW has no built-in serialization.
- **Fail-safe design**: silence on the wireless link is treated as a fault, not a "do nothing" state — communication loss always resolves to motor stop.

## References

- Random Nerd Tutorials — Getting Started with ESP-NOW (ESP32 with Arduino IDE)
- Random Nerd Tutorials — ESP-NOW Two-Way Communication Between ESP32 Boards
- ESP-NOW: The Ultimate Guide to Fast, Low-Power Communication with ESP32 & ESP8266
- *Developing IoT Projects with ESP32*
- *Electronics Projects with the ESP8266 and ESP32* (2020, Cameron)

## Author

- Malak Alsharqawi

**Partner:** Dana Natsheh

---
*Prepared for the IoT / Embedded Systems Training Program.*
