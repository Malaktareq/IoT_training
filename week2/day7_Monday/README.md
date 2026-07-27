# ESP32 Motor Control — Project 3
### Motor Driver & Modular Code (OOP)

**Platform:** ESP32-S3 · Arduino Framework · PlatformIO
**Subsystem:** L298N Dual H-Bridge Motor Driver

---

## 📌 Objective

Upgrade Project 2 by adding an **L298N Dual H-Bridge Motor Driver** for full speed (PWM) and direction control, and refactor the existing code into clean, modular C/C++ files inside PlatformIO.

This project introduces:
- A central configuration header (`config.h`)
- A **C-style** current sensor driver (no classes)
- A **C++ class-based** L298N motor driver
- A safety-first `main.cpp` with interrupt-driven E-Stop handling

---

## 🗂 Project Structure

```
my_project_folder/
├── platformio.ini          <- Hardware & environment configuration
├── include/                 <- Declarations & interface header files
│   ├── config.h              <- Pin mappings, limits, PWM constants
│   ├── CurrentSensor.h        <- C-style declarations
│   └── L298N.h                <- C++ class declaration
├── src/                      <- Function & class implementations
│   ├── CurrentSensor.cpp       <- C-style implementation
│   ├── L298N.cpp                <- Class method logic
│   └── main.cpp                  <- Entry point (setup & loop)
├── lib/                      <- Custom/third-party libraries (optional)
└── test/                     <- Unit testing scripts (optional)
```

**Rule of thumb:** Header files (`.h`) go in `include/`; implementation files (`.cpp`) go in `src/`.

---

## 🧩 File Specification Breakdown

### 1. `config.h` — Central Settings
Single source of truth for all hardware mapping and operational parameters. **Nothing should be hardcoded in `.cpp` files.**

Must define:
- Pin assignments for the L298N (`IN1`, `IN2`, `ENA`)
- Pin assignments for the Current Sensor, E-Stop button, and Warning LED
- Current safety threshold (in Amps)
- PWM properties (frequency, resolution/channel)

Use include guards (`#pragma once` or `#ifndef`/`#define`/`#endif`) at the top of every header.

---

### 2. `CurrentSensor.h` / `CurrentSensor.cpp` — C-Style Module
Handles analog current reading and overcurrent detection **without** C++ object instantiation.

```cpp
// Required C-style functions
void  CurrentSensor_Init(int pin, float sensitivity);
float CurrentSensor_ReadAmps(void);
bool  CurrentSensor_IsOvercurrent(float threshold);
```

Implementation notes:
- Average multiple ADC samples (e.g. 50 reads) before converting to voltage/current, to reduce raw sampling noise.
- Apply an **Exponential Moving Average (EMA)** low-pass filter to the computed current for additional smoothing (see [EMA Filter](#-low-pass-filter-ema) below).

---

### 3. `L298N.h` / `L298N.cpp` — C++ Class Driver
Encapsulates motor control state and direction/speed logic.

**Class Blueprint:**

| Access | Members |
|---|---|
| Private | `_in1Pin`, `_in2Pin`, `_enaPin`, `_speed`, `_direction` |
| Public | `L298N(int in1, int in2, int ena)` *(constructor)*, `begin()`, `forward(uint8_t speed)`, `reverse(uint8_t speed)`, `stop()`, `brake()` |

---

### 4. `main.cpp` — Application Logic
- Includes `config.h`, `CurrentSensor.h`, and `L298N.h`
- Instantiates the motor object and calls initialization functions in `setup()`
- Continuously monitors current draw and E-Stop state in `loop()`

**⚠️ Safety Rule:**
> Trigger an immediate stop/brake and light the warning LED if **Current > Threshold** OR **E-Stop is pressed**.

---

## ✅ PlatformIO Best Practices Checklist

- [ ] Use `#pragma once` (or `#ifndef`/`#define`/`#endif` guards) at the top of every `.h` file
- [ ] Maintain clean separation: declarations in `.h`, operational code in `.cpp`
- [ ] Include `#include "config.h"` in `.cpp` files wherever pin configs/parameters are needed

---

## 🔌 L298N H-Bridge Wiring & Pinout Guide

> Reference: [lastminuteengineers.com — L298N DC/Stepper Driver Arduino Tutorial](https://lastminuteengineers.com/l298n-dc-stepper-driver-arduino-tutorial/#onboard-5v-regulator-and-jumper)

```
+-------------------------------------------------------------+
|                        L298N MODULE                          |
|                                                               |
|  [ OUT1 ] [ OUT2 ]                    [ OUT3 ] [ OUT4 ]      |
|   (Motor A)                            (Motor B)             |
|                                                               |
|  [12V Power]  [GND]  [5V Logic Pin]                          |
|                                                               |
|  [ENA Jumper] [IN1] [IN2] [IN3] [IN4] [ENB Jumper]           |
+-------------------------------------------------------------+
```

### ⚠️ Critical Rule
Never omit the **common ground** connection — the ESP32-S3 and the external motor power supply must share a zero-volt ground reference.

### 1. Power & Ground Domain

| L298N Pin | Connected To | Purpose |
|---|---|---|
| 12V Terminal | External motor power (+) | High-current source for the DC motor (6V–12V) |
| GND Terminal | External power (–) **and** ESP32-S3 GND | Shared system ground reference |
| 5V Terminal | Unconnected (or 5V input if >12V supply) | Onboard 5V regulator output |

### 2. Microcontroller Control Domain (ESP32-S3)

| L298N Pin | ESP32-S3 Pin Type | Function | Behavior |
|---|---|---|---|
| ENA | PWM-capable GPIO | Speed control | Duty cycle 0–100% via PWM. **Remove jumper cap first!** |
| IN1 | Digital output GPIO | Direction control 1 | High/Low state line |
| IN2 | Digital output GPIO | Direction control 2 | High/Low state line |

### 3. Load & Sensing Loop (Motor A)

| Terminal | Path | Description |
|---|---|---|
| OUT1 | → ACS712 Terminal 1 | Current flows from OUT1 into the current sensor |
| ACS712 Terminal 2 | → DC Motor + | Sensor output passes to the positive motor terminal |
| OUT2 | → DC Motor – | Completes the H-bridge return loop |

### 📋 Pre-Power Verification Checklist
- [ ] **Common ground** — multimeter reads ≈0 Ω between ESP32-S3 GND and L298N GND
- [ ] **Series current path** — ACS712 sensor wired in series on the OUT1 line (not across power/ground)
- [ ] **ENA jumper** — physical jumper removed before attaching the PWM line
- [ ] **Floating pins** — IN1/IN2 explicitly mapped to digital output pins

---

## 📶 Low-Pass Filter (EMA)

Sensor readings are noisy due to ADC quantization, EMI, and supply fluctuation. An **Exponential Moving Average (EMA)** filter smooths successive readings using only one stored variable — ideal for embedded systems.

**Standard equation:**
```
y[n] = alpha * x[n] + (1 - alpha) * y[n-1]
```

**Optimized (single-multiply) form used in firmware:**
```cpp
filteredOutput += alpha * (newSample - filteredOutput);
```

| alpha | Response Speed | Noise Reduction |
|---|---|---|
| 0.05 | Very slow | Excellent |
| 0.10 | Slow | High |
| 0.20 | Balanced | Good |
| 0.50 | Fast | Moderate |
| 1.00 | Immediate | None |

For ESP32 current sensing, **alpha = 0.1–0.3** balances stability and responsiveness. The filter state must be declared `static` so the previous output persists between calls.

---

## ⚡ Interrupt-Driven E-Stop

Instead of polling the E-Stop pin every loop iteration, the emergency stop uses a **hardware interrupt** for guaranteed, immediate response.

```cpp
volatile bool emergencyStop = false;

void IRAM_ATTR emergency_stop_isr() {
    emergencyStop = true;   // minimal work inside the ISR
}

void setup() {
    attachInterrupt(digitalPinToInterrupt(E_STOP_PIN), emergency_stop_isr, FALLING);
}

void loop() {
    if (emergencyStop) {
        emergencyStop = false;
        motor.stop();
        Leds_Control(SystemState::EMERGENCY_STOP);
    }
}
```

**Key rules:**
- Keep the ISR as short as possible — only set a flag
- Avoid `delay()`, dynamic memory allocation, and `Serial.print()` inside an ISR
- Declare shared flags as `volatile` so the compiler doesn't cache stale values
- Mark ISRs with `IRAM_ATTR` on ESP32 so they run reliably even when Flash is busy

---

## 🔠 `enum class` vs. Traditional `enum`

This project standardizes on **`enum class : uint8_t`** for all state machines (motor state, system state).

| Feature | `enum` | `enum class` |
|---|---|---|
| Scope | Global | Inside enum |
| Type safety | Weak | Strong |
| Integer conversion | Automatic | Requires `static_cast` |
| Name conflicts | Possible | Prevented |
| Embedded usage | Acceptable | **Recommended** |

```cpp
enum class SystemState : uint8_t {
    STOP,
    RUNNING,
    FAULT_OVERCURRENT,
    EMERGENCY_STOP
};

SystemState state = SystemState::RUNNING;
```

Specifying `: uint8_t` reduces `sizeof(SystemState)` from the default 4 bytes to 1 byte — a meaningful RAM saving on microcontrollers.

---

## 🧠 Open Questions / Checkpoints

Before finalizing wiring and configuration, confirm:

1. Which exact GPIO pins on the ESP32-S3 are assigned to `IN1`, `IN2`, and `ENA` — and do they support hardware PWM output?
2. If pin definitions are placed as `const int PIN_X = 5;` inside `config.h` (without `constexpr` or `#define`), what linker error occurs when multiple `.cpp` files include that header?
3. How should `config.h` be structured so pin numbers and calibration parameters cannot be accidentally redefined across multiple `.cpp` files?
4. If `IN1 = HIGH` and `IN2 = HIGH` while `ENA` receives 100% PWM, what physically happens to current in the motor coils, and how does this differ from `IN1 = LOW` / `IN2 = LOW`?

---

## 📚 References

- [In-Depth: Interface L298N DC Motor Driver Module with Arduino](https://lastminuteengineers.com/l298n-dc-stepper-driver-arduino-tutorial/)
- [Beginner's Guide to Using Exponential Moving Averages (EMA) — Investing.com](https://www.investing.com/academy/analysis/beginners-guide-to-using-exponential-moving-averages/)
- Exponential Moving Averages Explained Simply
- [Applying Filters in Embedded Technology — Smowcode Blog](https://blog.smowcode.com/applying-filters-in-embedded-technology/)
- [Simplifying DSP Filters for Embedded Systems — DMC, Inc.](https://www.dmcinfo.com/blog/40102/simplifying-dsp-filters-for-embedded-systems/)

---

## ✅ Summary Checklist

- [ ] `config.h` created with all pins, thresholds, and PWM settings
- [ ] `CurrentSensor.h/.cpp` implemented as C-style functions (no class)
- [ ] `L298N.h/.cpp` implemented as a C++ class with constructor + 5 public methods
- [ ] H-bridge wired and verified against pre-power checklist
- [ ] EMA filter applied to current readings (alpha = 0.1–0.3)
- [ ] E-Stop converted to hardware interrupt with `volatile` flag
- [ ] All state enums converted to `enum class : uint8_t`
- [ ] `main.cpp` stops/brakes motor + lights warning LED on overcurrent OR E-Stop
