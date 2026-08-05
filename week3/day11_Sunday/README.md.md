# ESP32 MQTT Two-Board Pairing (Project 4)

IoT / Embedded Systems training project — symmetric two-topic MQTT pairing between two identically-built ESP32 boards, extending the ESP-NOW pairing from Project 3 into a publish/subscribe architecture over a public broker.

**Team:** Malak Alsharqawi · Dana Natsheh

---

## Overview

This project replaces the direct, point-to-point ESP-NOW link from Project 3 with an MQTT publish/subscribe link. Two ESP32 boards, owned by two different students, communicate indirectly through a shared public broker instead of a direct MAC-address connection.

Both boards are built identically — each carries its own **IR obstacle sensor** and its own **L298N-driven DC motor**. Each board publishes its own sensor reading and subscribes to its partner's reading, using that reading to control its own motor:

- When **Board A's IR sensor detects an object**, **Board B's motor stops**.
- When **Board B's IR sensor detects an object**, **Board A's motor stops**.
- When either board's path is clear, the *other* board's motor drives forward.

Both boards act as publisher and subscriber at the same time, using two separate topics — one per direction — so the pairing is fully symmetric.

## Why MQTT

| Aspect | ESP-NOW | HTTP | MQTT |
|---|---|---|---|
| Addressing | Direct MAC address | URL / server address | Topic name only |
| Coupling | Sender must know receiver's MAC | Client must know server address | Publisher/subscriber never know about each other |
| Style | Direct device-to-device | Request / response | Publish / subscribe, event-driven |
| Range | Local Wi-Fi only | Anywhere with network access | Anywhere with internet access, via a broker |
| Overhead | Very low, local only | Higher (headers, method, version) | Very low, application layer only |

MQTT is event-driven rather than request-driven: as soon as either board's IR sensor state changes, the update reaches the partner board automatically, with no polling required.

## Broker

- **Primary:** `broker.emqx.io`
- **Alternative:** `iot.coreflux.cloud`

A public broker is used so the pairing works over the open internet rather than only on a local Wi-Fi network. No authentication is required for this classroom exercise.

## Architecture — Symmetric Two-Topic Pairing

Each board publishes its own IR sensor reading on one topic and subscribes to its partner's IR sensor reading on the other, using that reading to drive its own, locally-attached motor. Swapping the two topic names between boards is what creates the pairing (analogous to swapping MAC addresses in the ESP-NOW version).

| Board | Hardware | Publishes On | Subscribes To | Action on Receive |
|---|---|---|---|---|
| Board A (Student 1) | IR obstacle sensor + L298N DC motor (identical to Board B) | Board A IR-status topic | Board B IR-status topic | Stops its own motor when Board B's IR reports a detection; drives forward when Board B reports a clear path |
| Board B (Student 2) | IR obstacle sensor + L298N DC motor (identical to Board A) | Board B IR-status topic | Board A IR-status topic | Stops its own motor when Board A's IR reports a detection; drives forward when Board A reports a clear path |

Both boards run identical firmware; only the settings file (topic assignments) differs per board.

## File Structure

```
├── config.h          # Wi-Fi credentials, broker host/port, topic names, pin definitions
├── MQTTHandler.h/.cpp # MQTT class: wraps WiFiClient/PubSubClient, connect/publish/subscribe/callback
├── L298N.h/.cpp       # Motor driver class (reused unchanged from Project 3)
└── main.cpp           # Setup + board-specific decision logic only
```

- Central `config.h` holds every credential, pin, topic string, and timing constant — nothing is hard-coded in implementation files.
- The MQTT class owns the network client, handles connecting, publishing, and exposes the callback hook.
- The `L298N` class is reused unchanged from Project 3 for motor direction/speed control.
- The main application file makes no direct calls into the underlying network/MQTT library — only through the MQTT class's API.

## Key Implementation Details

**Topic guard** — Since both boards subscribe to one topic and publish on another, the incoming-message handler checks which topic a message actually arrived on before acting, so a board never reacts to messages not meant for it.

**Safety watchdog** — Each board tracks the last time it received a valid update from its partner. If no update arrives within a short, fixed timeout, that board's own motor is force-stopped and its stop indicator lit — protecting against lost Wi-Fi, a dropped broker connection, or a crashed partner board. Neither board simply keeps repeating the last known command indefinitely.

**Message → motor mapping (per board):**
- Partner reports **object detected** → stop indicator on, running indicator off, **local motor stops**
- Partner reports **path clear** → running indicator on, stop indicator off, **local motor drives forward** at full speed
- Unrecognized values are currently ignored (known gap, see Future Improvements)

## End-to-End Flow

1. Board A reads its own IR sensor and evaluates detected/clear.
2. Board A publishes that state to the Board A IR-status topic.
3. The broker checks its subscription table for that topic.
4. The broker forwards the message to Board B.
5. Board B verifies the message arrived on its subscribed topic, updates its last-received timestamp, and sets its own motor/LED state accordingly — stopping its motor if Board A reports a detection, or driving it forward if Board A reports a clear path.
6. Board B symmetrically publishes its own IR reading on the Board B IR-status topic; Board A reacts the same way, controlling its own motor from Board B's reading.
7. If no message arrives within the timeout, the receiving board's watchdog forces a safety stop independently of the last received command.

## Getting Started

1. Rehearse the pub/sub flow manually first using a browser-based MQTT client (e.g. [MQTTX Web Client](https://mqttx.app/web-client)) connected to `broker.emqx.io`, to confirm broker/topic behavior before touching firmware.
2. Flash each board with the shared firmware, setting that board's `config.h` with:
   - Wi-Fi SSID/password
   - Broker host and port
   - Its own publish topic (its IR-status topic) and its subscribe topic (its partner's IR-status topic)
   - Pin assignments for both the IR sensor and the L298N motor driver — every board has both
3. Power on both boards and verify via the web client (or serial monitor) that a detection on one board's sensor stops the other board's motor, and that the watchdog stops the motor if updates stop.

## Dependencies

- [PubSubClient](https://github.com/knolleary/pubsubclient) (Arduino MQTT client library)
- ESP32 Arduino core `WiFi.h`
- PlatformIO

## Future Improvements

- Handle unrecognized/unexpected message payloads explicitly instead of silently ignoring them
- Add reconnect logic for dropped broker connections
- Add TLS for the broker connection

## References

- [MQTT.org](https://mqtt.org/)
- [GeeksforGeeks — Introduction to MQTT](https://www.geeksforgeeks.org/)
- [EMQX — Easiest Guide to Getting Started with MQTT](https://www.emqx.com/en/blog/the-easiest-guide-to-getting-started-with-mqtt)
- [EMQX — Ultimate Guide to MQTT Broker Comparison](https://www.emqx.com/en/blog/the-ultimate-guide-to-mqtt-broker-comparison)
- [PubSubClient MQTT Arduino Guide](https://pcbsync.com/pubsubclient-mqtt-arduino/)
- [Steve's Internet Guide — Arduino PubSub MQTT Client](http://www.steves-internet-guide.com/using-arduino-pubsub-mqtt-client/)

---
*Confidential — Internal Training Documentation, IoT Training Program*
