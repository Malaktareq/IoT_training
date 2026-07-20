# Day 2 — IR Sensor, LED & Potentiometer (PWM Brightness Control)

**Project:** IoT Training Program
**Board:** ESP32 DEVKIT V1 – DOIT (36 GPIOs)
**Date:** 20/7/2026
**Prepared By:** Malak Alsharqawi — Cyber Robot

## Overview

This task combines digital sensing and analog control on the ESP32:

- An **IR obstacle sensor** acts as a presence/proximity gate.
- A **potentiometer** sets LED brightness via PWM.
- The **LED** only lights up (at the brightness set by the pot) when the IR sensor detects an object in range; otherwise it stays off.

Along the way this also covers the ESP32's 12-bit ADC (0–4095), 8-bit PWM resolution (0–255), and a boot-time upload failure caused by wiring the potentiometer to a strapping pin.

## Hardware & Wiring

| Component | Pin(s) | Notes |
|---|---|---|
| IR sensor | VCC, GND, Vout → GPIO 18 | Digital input; outputs `LOW` when an object is detected. Onboard trimpot sets detection distance. |
| Potentiometer | Outer pins → 3.3V & GND; wiper → GPIO 34 | GPIO 34 is a true input-only **ADC1** channel — safe to use, no boot-strapping role. |
| LED | GPIO 19 (+ current-limiting resistor) | PWM output via `analogWrite()`. |

> ⚠️ **Do not use GPIO 12 for the potentiometer.** It's a strapping pin — see [Issues & Troubleshooting](#issues--troubleshooting).

## How the Firmware Works

1. Read the IR sensor digitally (`digitalRead`) and the potentiometer as a 12-bit analog value (`analogRead`, 0–4095).
2. Map the 0–4095 potentiometer range to the 0–255 PWM range with `map()`.
3. If the IR sensor reports an object (`LOW`): drive the LED at the mapped brightness.
4. If the path is clear: force the LED off, regardless of the potentiometer position.
5. Log IR state, raw pot value, and computed voltage to Serial every ~100 ms.

### Code (`main.cpp`)

```cpp
#include <Arduino.h>

#define IR_PIN 18
#define LED_PIN 19
#define POT_PIN 34

int flag = 1;
void setup() {
  pinMode(IR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);

  Serial.begin(115200);
  Serial.println("Checking IR sensor...");
}

void loop() {
  int irValue = digitalRead(IR_PIN);
  int potValue = analogRead(POT_PIN);
  int Brightness = map(potValue, 0, 4095, 0, 255);
  float voltage = (potValue / 4095.0) * 3.3;

  if (irValue == LOW) {
    analogWrite(LED_PIN, Brightness);
    if (flag == 0) {
      flag = 1;
      Serial.println("IR sensor detected an object!");
    }
  } else {
    analogWrite(LED_PIN, 0);
    if (flag == 1) {
      flag = 0;
      Serial.println("IR sensor did not detect any object.");
    }
  }

  Serial.print("IR Reading: ");
  Serial.print(irValue == LOW ? "OBJECT" : "CLEAR ");
  Serial.print(" | Pot Value: ");
  Serial.print(potValue);
  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.println("V");
  delay(100);
}
```

### `platformio.ini`

```ini
[env:esp32dev]
platform = espressif32
board = esp32dev
framework = arduino
monitor_speed = 115200
```

## Why 4095 and 255?

- **4095** — the ESP32's ADC is 12-bit, giving 2¹² = 4096 steps (0–4095), mapped to 0–3.3V.
- **255** — `analogWrite()` uses 8-bit PWM resolution, giving 2⁸ = 256 steps (0–255), where 255 = 100% duty cycle.
- `map(potValue, 0, 4095, 0, 255)` translates the knob position onto the LED's power scale.

## Issues & Troubleshooting

**Symptom:**
```
Failed to communicate with the flash chip, read/write operations will fail
```

**Root cause:** GPIO 12 is a *strapping pin* — the ESP32 checks its electrical state at boot/flash time to decide the internal flash chip's voltage (`LOW` → 3.3V, `HIGH` → 1.8V). With the potentiometer originally wired to GPIO 12, turning the dial could leave the pin `HIGH`, forcing the flash chip into 1.8V mode and starving it of the 3.3V it actually needs — breaking upload/flash communication.

**Fix:** Move the potentiometer's wiper to **GPIO 34** (input-only ADC1 pin, no strapping role). This immediately resolved the upload failure.

## Results

- LED brightness tracks the potentiometer smoothly (0–255) whenever the IR sensor detects an object.
- LED switches off instantly when the path is clear, regardless of pot position.
- Serial monitor (115200 baud) confirms IR state (`OBJECT`/`CLEAR`), raw pot value, and voltage (2 decimals) about 10×/second.

## References (verified live)

- [ESP32 Pinout Reference (GPIOs)](hhttps://lastminuteengineers.com/esp32-pinout-reference/) — pin capabilities, strapping pins, ADC channels
- [The Potentiometer: Pinout, Wiring, and How It Works](https://www.build-electronic-circuits.com/potentiometer/)
- [Arduino LED Dimmer (Potentiometer + PWM)](https://leecuriosity.com/arduino-led-dimmer-potentiometer-pwm/)
- [Development Notes & Key Learnings (Notion)](https://app.notion.com/p/Day-1-3a254ef921fa80769a0dc1db6c03625e?source=copy_link)

---
*Full write-up with diagrams and photos: [Full document](https://docs.google.com/document/d/1ikk3YVCFnpApnltb2eqKkvR_lR3JvQQw/edit?usp=sharing&ouid=104574417241291360945&rtpof=true&sd=true)