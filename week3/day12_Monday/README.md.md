# Embedded Data Structures and IoT Communication Protocols

IoT / Embedded Systems training report covering C++ data layout on the ESP32 (enums, struct padding and alignment, packed data, and unions) and a comparative study of HTTP, MQTT, and ESP-NOW.

**Prepared by:** Malak Alsharqawi · Cyber Robot
**Program:** IoT Training Program
**Companion report:** ESP32 Ecosystem, Communication Protocols, and Power Sizing

---

## Overview

This report is a two-part deep dive that connects how a struct is laid out in memory to how those exact bytes travel over the air. Section 1 covers the C++ data structures used throughout the training program's firmware; Section 2 covers the three communication protocols used to move that data between devices. The link between them is ESP-NOW, which transmits raw application bytes with no serialisation layer of its own — so the layout decisions in Section 1 become the wire format in Section 2.

Every acronym is spelled out on first use, every concept opens with a plain-language explanation or analogy before the formal definition, and every platform-specific claim is tied explicitly to the ESP32 rather than presented as a universal rule.

## Section 1: Structs, Enums, and Unpacking Data

**1.1–1.3 Enums and scoped enums (`enum class`)**
- Why bare integers (`int motorState = 2`) are worse than named enumerators.
- How enumerator values are assigned automatically or explicitly.
- Underlying type and size (`enum Color : uint8_t` → 1 byte instead of 4).
- The two structural problems of C-style (unscoped) enums: name pollution and implicit conversion to `int`.
- Six concrete reasons `enum class` is safer for state machines: no name collisions between coexisting state machines, no comparison against raw numbers, no comparison between unrelated state types, no accidental arithmetic, predictable one-byte size, and intact `-Wswitch` exhaustiveness checking.
- The one thing `enum class` does *not* do: validate values arriving from outside the program (a packet, flash, etc.) — those still need an explicit boundary check.
- A worked example: a one-byte, type-safe, compiler-checked motor state machine.

**1.4 Struct padding and memory alignment**
- Four foundational definitions: memory address, size (`sizeof()`), alignment (`alignof()`), and padding.
- Why alignment exists: a CPU reads memory in word-sized chunks, and misaligned reads cost extra instructions or fault outright.
- Padding between members and end padding (why arrays force it).
- How simply reordering struct members — largest alignment requirement first, smallest last — can cut RAM usage by a third at zero runtime cost.
- Why the ABI (not the CPU) decides alignment rules, and why the same struct can have a different `sizeof()` on the ESP32 versus a desktop, or between Xtensa and RISC-V ESP variants.
- Tools for inspecting layout: `sizeof()`, `alignof()`, `offsetof()`, and locking the contract with `static_assert`.

**1.5 `__attribute__((packed))`**
- What packing does (removes padding) and the four things it does *not* do (shrink types, change endianness, provide portability, or make the format self-describing).
- The cost: unaligned members, and what that means specifically on ESP32 Xtensa/RISC-V vs. external PSRAM.
- A decision table for when to pack and when not to.
- A worked ESP-NOW telemetry packet example with a `static_assert` size contract.
- The more portable alternative: explicit byte-by-byte serialisation.

**1.6 Unions and byte views**
- The three reasons unions appear in embedded code: saving RAM, viewing the same bytes two ways (e.g. extracting a `float`'s raw bytes), and mapping hardware registers.
- Two caveats: type punning through unions is a GCC extension (not strict standard C++), and bitfield layout is implementation-defined — never use bitfields as a cross-compiler wire format.

## Section 2: IoT Communication Protocols (HTTP, MQTT, ESP-NOW)

**2.1 HTTP** — stateless client/server request-response over TCP/IP. Universal and easy to debug, but no server push (forces polling) and heavy per-request header overhead.

**2.2 MQTT** — publish/subscribe over TCP/IP via a broker. Covers topic hierarchies and wildcards (`+`, `#`), the three QoS levels (at most once / at least once / exactly once), and the features with no HTTP equivalent: retained messages, Last Will and Testament, persistent sessions, and a fixed header as small as two bytes.

**2.3 ESP-NOW** — Espressif's connectionless, broker-free, router-free protocol riding directly on the 802.11 MAC layer. Covers the hard numeric limits (250-byte payload in v1.0, 1,470 bytes in v2.0; up to 20 paired peers, 17 encrypted; CCMP/AES-128 encryption with no broadcast encryption) and two commonly-wrong claims: unicast frames *do* get a MAC-layer delivery signal via the send callback (though not an application-level ack), and ESP-NOW peers must share a Wi-Fi channel — which is dictated by the access point on any device that's also a Wi-Fi client.

**2.4–2.6 Comparison and decision guidance** — a master comparison table across every criterion that drives protocol choice (addressing, overhead, latency, power, range, security, scalability), why MQTT overtakes HTTP once messages repeat over a connection, why ESP-NOW's latency/power profile is in a different category entirely (no association, DHCP, or TCP handshake), and a short decision path for choosing between the three.

**2.7 The hybrid pattern** — the real-world answer: ESP-NOW at the battery-powered sensor edge, an MQTT-connected gateway in the middle, and HTTP wherever a person is looking at a screen.

**2.8 Applied example** — maps all of Section 1 and Section 2 together in one ESP-NOW interlock payload: a scoped `enum class` motor state, a packed struct, and a `static_assert` size contract, all inside the 250-byte v1.0 limit.

**2.9 Common mistakes checklist** — eight field-tested pitfalls, including sending unpacked structs over ESP-NOW, ignoring the channel constraint, treating the send callback as an application-level ack, polling HTTP on a battery device, overusing QoS 2, and expecting broadcast frames to be encrypted.

## Key Takeaways

- Prefer `enum class` over a plain `enum` in all new C++ code, especially for states, modes, commands, and error codes.
- Order struct members from largest alignment requirement to smallest to minimize padding — free RAM savings with zero behavior change.
- `__attribute__((packed))` is for structs whose layout is dictated externally (a wire format, a register map, a file spec) — not a general RAM-saving trick.
- Any struct that defines a wire format should carry a `static_assert` on its size.
- Choose the protocol for the job, not one protocol for the whole system: HTTP for occasional human-facing interaction, MQTT for continuous many-to-one telemetry and cloud integration, ESP-NOW for latency-critical or infrastructure-free local links.

## References

Full citations are listed in the report's References section, grouped as:
- **Primary documentation** — Espressif ESP-IDF Programming Guide (ESP-NOW), Espressif migration guides and example code, OASIS MQTT v5.0 spec, cppreference, Microsoft Learn, SEI CERT C++ Coding Standard.
- **Protocol references and articles** — HiveMQ (MQTT Essentials, MQTT vs HTTP, MQTT Packets), EMQX, Arduino documentation.
- **Memory layout, structs, unions and enums** — GeeksforGeeks, Modernes C++ (C++ Core Guidelines), and various technical articles on struct padding and alignment.

---
*Internal Training Documentation — IoT Training Program*
