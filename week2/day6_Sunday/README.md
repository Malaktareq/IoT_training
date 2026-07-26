<div align="center">

# 📅 Day 6
### ESP32 Memory Architecture & Embedded C/C++ Concepts

*A headline-only recap of today's notes*

</div>

---

## 🏗️ ESP32 Memory Architecture

| | |
|---|---|
| 🧠 | Internal vs External Memory |
| 📦 | ESP32 Module Layout |
| 🗺️ | ESP32 Memory Types |
| 📖 | ROM (Read Only Memory) |
| ⚡ | IRAM (Instruction RAM) |
| 💾 | DRAM (Data RAM) |
| 🌙 | RTC Fast Memory |
| 🌙 | RTC Slow Memory |
| 💽 | Flash Memory |
| 💿 | PSRAM |
| ⚙️ | Cache Memory |
| 🚀 | Where Does My Code Actually Live? *(Execute In Place / XIP)* |
| ❓ | Then Why Does IRAM Exist? |
| 📌 | Does IRAM Store My Entire Program? |
| ✂️ | How the Program Is Split |
| 📝 | What Does ".text" Mean? |

---

## 🔄 C/C++ Memory Sections in Embedded Systems

| | |
|---|---|
| 1️⃣ | Source Code → Compilation → Linking → Binary |
| 📄 | What Is an ELF File? |
| 🏙️ | Complete MCU Memory Layout |
| `.text` | Executable machine instructions |
| `.rodata` | Read-only constants |
| `.data` | Initialized global variables |
| `.bss` | Uninitialized global variables |
| 📥 | Stack |
| 📤 | Heap |
| ⚖️ | Stack vs Heap Example |
| 📍 | Variable Memory Locations |
| 🔍 | Inspecting Sections — `readelf`, `objdump`, `nm` |
| 🛠️ | ESP-IDF Build Output |

---

## 🔑 ESP32 Directives & Keywords

| | |
|---|---|
| 🧊 | `volatile` — why it exists |
| 🤖 | Why Does the Compiler Optimize Variables? |
| ✅ | How `volatile` Solves the Problem |
| 🚫 | What Optimizations Does `volatile` Prevent? |
| ⚡ | Why Is `volatile` Required for ISRs? |
| 🏷️ | `IRAM_ATTR` — what it does |
| 🔌 | When Does Flash Become Unavailable? |
| 💥 | What Happens If an Interrupt Occurs? |
| 🩹 | How `IRAM_ATTR` Solves the Problem |
| ⚠️ | What Happens If `IRAM_ATTR` Is Omitted? |
| 📊 | Comparison Table: `volatile` vs `IRAM_ATTR` |

---

## 🎯 Key Takeaways

- 🧠 **Internal** memory (fast, small) vs **External** memory (large, slower)
- 🚀 Most code runs **from Flash** via **Execute-In-Place (XIP)**
- ⚡ **IRAM** is reserved for interrupts & critical timing code
- 💾 **DRAM** holds runtime data — variables, stack, and heap
- 📝 The `.text` section vs the IRAM **"Text"** region are *not* the same thing

