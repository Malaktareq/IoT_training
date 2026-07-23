# Project 2 — Simple Motor Protection System

An ESP32-S3 based protection circuit for a DC motor. It monitors motor current in real time with a Hall-effect current sensor, cuts power through a relay the instant current exceeds a safe threshold, and gives an operator an instant hardware E-Stop — all backed by a 4-LED status panel so the system's state is visible at a glance.

## Task Brief

- Power the motor from an external power supply.
- Monitor motor current continuously. If it exceeds a set limit, cut power via a relay. If it stays within range, keep the motor running.
- Provide a push-button E-Stop.
- Show system state (Running / Stopped / High Current / Low Current) on LEDs.

## Definition of Done

| Requirement | Status | How it's met |
|---|---|---|
| E-Stop stops the motor instantly | ✅ | Button press sets `eStopTriggered` and immediately writes the relay pin OFF in the same loop iteration (also checked mid-sample, so it can't be delayed by ADC averaging) |
| Overcurrent protection trips the relay | ✅ | Current is computed every loop from 50 averaged ADC samples; crossing `OVERCURRENT_LIMIT` sets `overcurrentFault` and cuts the relay |
| LEDs wired correctly with proper resistors | ✅ | 4 LEDs (Running / Stopped / Low Current / High Current), each behind a current-limiting resistor — see [Wiring](#wiring--pinout) |
| Code is clean and easy to modify | ✅ | All thresholds/pins are named constants at the top of the file; state changes go through one `updateLEDs()` helper |
| Clear, readable wiring schematic | ✅ | See [Schematics & Photos](#schematics--photos) — pinout reference + component diagrams included; add your own hand-drawn/Fritzing schematic alongside them |

## Hardware / Bill of Materials

| Component | Model | Purpose |
|---|---|---|
| Microcontroller | ESP32-S3 (DevKitC-1 / N16R8) | Reads current, drives relay & LEDs |
| Current Sensor | HW-872A (Allegro ACS712ELCTR-05B-T, ±5A) | Real-time Hall-effect current sensing, in series with the motor |
| Relay | SRD-05VDC-SL-C (Active-LOW, 10A rated) | Switches high-power motor circuit on/off |
| Push Button | 4-pin tactile switch | Dual-function E-Stop / Reset input |
| LEDs (x4) | 5mm, any color per channel | Running (green), Stopped (red), Low Current (blue), High Current fault (yellow) |
| Resistors (x4) | ~220–330Ω | Current-limiting resistors in series with each LED |
| Load | DC Motor | The protected device |
| Power | USB / ESP32 5V rail + external DC supply | Logic power vs. motor drive power (kept separate, common ground) |

## Wiring & Pinout

### Low-Voltage Logic Domain (ESP32-S3 side)

| Signal | ESP32-S3 Pin | Notes |
|---|---|---|
| Current sensor `OUT` | GPIO 4 | Analog input, `ADC1_CH3`, `ADC_11db` attenuation |
| Current sensor `VCC` / `GND` | 3.3V / GND | Powered directly from the 3.3V rail (no divider needed) |
| Relay `IN` | GPIO 5 | Digital output, Active-LOW (LOW = relay energized) |
| Relay `VCC` / `GND` | 5V / GND | Coil power |
| Push button (diagonal leg) | GPIO 6 | `INPUT_PULLUP` — wire across **opposite diagonal pins** on the 4-pin switch, never the same internally-shorted side |
| Push button (other diagonal leg) | GND | |
| Green LED — Running | GPIO 15 | Through resistor to GND |
| Red LED — Stopped | GPIO 7 | Through resistor to GND |
| Blue LED — Low/Normal Current | GPIO 17 | Through resistor to GND |
| Yellow LED — High Current / Fault | GPIO 16 | Through resistor to GND |

### High-Current Power Domain (motor circuit)

```
[ Power Supply (+) ] ---> Relay [ COM ]
   Relay [ NO ]         ---> Current Sensor Terminal 1 (IP+)
   Current Sensor Terminal 2 (IP-) ---> Motor (+)

[ Power Supply (-) ] --------------------------------> Motor (-) ---> Common GND
```

> **Important — shared ground:** the external power supply ground, the motor ground, and the ESP32-S3 GND must all be tied together. Without a common ground, the current sensor's analog reading on GPIO 4 will be inaccurate or unstable.

> **Important — NO, not NC:** the motor must be wired to the relay's **Normally Open (NO)** contact. If wired to NC, losing power or the microcontroller crashing would leave the motor running instead of stopping — the opposite of fail-safe.

## System Behavior

| State | Relay | Green (Run) | Red (Stop) | Blue (Low) | Yellow (Fault) |
|---|---|---|---|---|---|
| Boot | OFF | OFF | ON | OFF | OFF |
| Normal running (I < limit) | ON | ON | OFF | ON | OFF |
| Overcurrent trip (I ≥ limit) | OFF | OFF | ON | OFF | ON |
| E-Stop active | OFF | OFF | ON | OFF | OFF |

The push button is dual-purpose:
- **While running:** a press is an **E-Stop** — it cuts the relay immediately and latches the system in a stopped state.
- **While stopped or faulted:** a press is a **Reset** — it clears the fault flags and restarts the motor.

Both fault conditions (E-Stop and overcurrent) **latch** — the relay stays OFF until the button is pressed again, so the motor never silently restarts on its own.

## Calibration

The current sensor is powered from 3.3V instead of its nominal 5V, so both its zero-current baseline and its sensitivity must be re-derived rather than taken from the datasheet:

- **Baseline (0A voltage):** measured empirically with no load connected (~1.57V in testing — re-measure for your own board).
- **Scaled sensitivity:** `S(3.3V) = S(5V) × (3.3 / 5.0) ≈ 0.1221 V/A` (vs. the datasheet's 0.185 V/A at 5V).
- **Overcurrent limit:** set in firmware as `OVERCURRENT_LIMIT` — pick a value comfortably above your motor's normal running current and below its stall/inrush current. 300 mA was used for a small test motor; **size this to your actual motor's rated current.**

To (re)calibrate on your own hardware:
1. Power the board with no motor load connected.
2. Read and average the raw ADC value over a few seconds → convert to voltage → that's your `RESTING_OFFSET`.
3. Update `RESTING_OFFSET`, `SENSITIVITY`, and `OVERCURRENT_LIMIT` at the top of the firmware.

## Firmware

Firmware constants live at the top of the sketch so the whole system can be retuned without touching any logic:

```cpp
const int SENSOR_PIN = 4;
const int RELAY_PIN  = 5;
const int ESTOP_PIN  = 6;
const int LED_RUNNING   = 15;
const int LED_STOPPED   = 7;
const int LED_LOW_CURR  = 17;
const int LED_HIGH_CURR = 16;

const float SENSITIVITY       = 0.1221; // V/A, scaled for 3.3V supply
const float RESTING_OFFSET    = 1.572;  // V, measured 0A baseline
const float OVERCURRENT_LIMIT = 0.300;  // A, trip threshold — set to your motor
```

Core loop, each cycle:
1. Poll the push button for a falling edge (debounced ~50ms) → toggle E-Stop / Reset.
2. If `eStopTriggered` or `overcurrentFault` is set → force relay OFF, set LEDs, and `return` (skip sensing) until reset.
3. Otherwise, energize the relay, average 50 ADC samples from the current sensor (checking the button mid-loop so an E-Stop is never delayed by sampling), convert to current, and update the LEDs.
4. If current ≥ `OVERCURRENT_LIMIT`, trip the fault and cut the relay.

The full annotated sketch and a line-by-line breakdown are in the accompanying **Day 4 Technical Report**.

## Setup / Flashing

1. Wire the circuit per the [Wiring & Pinout](#wiring--pinout) section — **do this with the power supply and motor disconnected.**
2. Flash the firmware to the ESP32-S3 (Arduino/PlatformIO, 115200 baud).
3. With the motor disconnected, power on and confirm: Red LED lit, relay OFF, `--- MOTOR PROTECTION SYSTEM READY ---` on serial.
4. Reconnect the motor circuit. Press the button once — relay should click ON, Green + Blue LEDs light, motor spins.
5. Press the button again — relay should click OFF instantly, Red LED lit. This is your E-Stop check.
6. Reset, then briefly stall or load the motor to push current above `OVERCURRENT_LIMIT` — relay should trip OFF on its own, Red + Yellow LEDs lit, and stay off until you press the button to reset.



## Safety Notes

- Always wire the motor to the relay's **NO** contact, never **NC** (see [Wiring](#wiring--pinout)).
- Size `OVERCURRENT_LIMIT` to your actual motor and load — the value in this repo is a placeholder from bench testing with a small test motor.
- Keep the high-current motor wiring physically separated from the low-voltage logic wiring; the current sensor provides galvanic isolation, but keep connections clean and shared ground solid.
- This system protects against sustained overcurrent and provides a manual E-Stop; it does not replace a mechanical/physical emergency stop for anything driving real mechanical loads.
