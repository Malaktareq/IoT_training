<div align="center">

# 📘 ESP32 Architecture & Embedded C/C++ Concepts
### Assignment Brief & Deliverable Summary

`Submission: PDF / Google Doc → Trello` · `Type: Research Report`

</div>

---

## 📖 Description

This assignment investigates **ESP32 memory spaces**, **C/C++ memory mechanics**, **OOP principles**, and **basic input/power protection concepts**, to prepare for production driver development and FreeRTOS. The full write-up is delivered as **`ESP32_Architecture_Embedded_CPP_Report.docx`**, covering all nine questions below across three sections, with diagrams, comparison tables, and code examples.

---

## 🏗️ Section 1 — ESP32 Memory Architecture & Layout

**1. The ESP32 Memory Map**
Research the physical memory layout of the ESP32 and explain the role, typical capacity, speed characteristics, and practical use cases of:
- 💾 DRAM (Data RAM)
- ⚡ IRAM (Instruction RAM)
- 💿 PSRAM (External SPI RAM)

**2. General MCU Memory Sections**
Define the standard C/C++ memory segments — `.text`, `.rodata`, `.data`, `.bss`, Stack, and Heap — and determine which segment each of these lives in:
- `const int x = 5;`
- Global `int y = 5;`
- Global `int z;`
- Local variable inside a function: `int k = 10;`
- A buffer created via `malloc()` or `new`

**3. ESP32 Directives & Keywords**
Research the technical purpose and execution mechanics of:
- 🧊 `volatile` — which compiler optimizations it overrides, and why it's needed for ISRs
- 🏷️ `IRAM_ATTR` — where it forces code to execute, and what happens if it's omitted in an ISR

---

## 🧭 Section 2 — Pointers, References & Memory Management

**4. Pointers vs. References (C vs. C++)**
- 📊 Comparison table: Pointers (`*`) vs. References (`&`) — Nullability, Reassignment, Pointer Arithmetic, Syntax
- ⚙️ Performance: why `const SensorData &data` beats pass-by-value or pass-by-raw-pointer

**5. Structs, Enums, and Unpacking Data**
- 🔢 Enums: C-style `enum` vs. C++ `enum class` (Scoped Enums), and why scoped enums are safer for state machines
- 🧱 Alignment: what Struct Padding & Alignment is, how it affects RAM usage, and what `__attribute__((packed))` does

**6. Function Pointers & Callbacks**
- 🎯 What a Function Pointer is
- 🔔 How function pointers implement event-driven callbacks (e.g., a button-press interrupt or timer expiry)

---

## 🧩 Section 3 — C++ OOP & Access Modifiers in Embedded Systems

**7. Access Specifiers (`public`, `private`, `protected`)**
- 🔒 Encapsulation, and why hardware pins / sensor calibration values should stay `private`
- 🧬 The practical difference between `private` and `protected` when inheriting a base driver class

**8. Constructors & Initialization Lists**
- 🏗️ The purpose of a C++ Constructor and Destructor
- 📋 Why Member Initializer Lists are preferred over assigning inside the constructor body

**9. Cost of C++ Features in Embedded Systems**
- 🧮 Memory and CPU overhead of virtual functions and the vtable
- 🗑️ Dynamic memory allocation (`new` / `delete` / `malloc` / `free`) on microcontrollers — Stack vs. Heap, fragmentation, and non-determinism

---

<div align="center">

---
*📚 Part of the IoT / Embedded Systems Training Program — Cyber Robot*

</div>
