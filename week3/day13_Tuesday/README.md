# Isolation, Protection and EMI Control in ESP32 Motor Control Systems

A beginner-friendly technical report on the three components that sit between a microcontroller and a motor, how to stop a motor from destroying the microcontroller that commands it, and the practical minimum of EMI control for a small embedded project.

Written for the **IoT Training Program, Day 13**.

---

## What this covers

An ESP32 runs on milliamps. A motor runs on amps. Something has to stand between them. This report explains what that something is, why there are three different candidates, and which one belongs at which point in an ESP32 and L298N circuit.

Every term is defined the first time it appears, and every concept opens with an everyday analogy before the engineering explanation. No prior electronics background is assumed.

### Section 1: Relay vs. Optocoupler vs. Solid-State Relay

All three isolate two circuits, which is exactly why beginners confuse them. Isolation is a shared feature, not a purpose. The section covers what is physically inside each device, how each one works step by step, a full comparison matrix, and then the applied question: which device belongs on the L298N enable and direction lines, and which belongs on the motor's main power line.

### Section 2: Protecting the ESP32

Three defences against three different failure mechanisms.

- **Flyback diodes.** Why interrupting current through a coil produces a voltage spike many times the supply voltage, where the diode goes and why it must be in parallel with the load, and why most L298N modules already contain theirs.
- **Optocoupler isolation boards.** Why a GPIO wire is not a one-way road, and why isolation matters even when the ESP32 is only transmitting.
- **Power supply isolation.** Why a motor stall drags a shared rail down, worked through with Ohm's law, and how to split the branches so it does not reach the logic side.

### Section 3: EMI, The Practical Minimum

Where the noise comes from in this specific setup, the two paths it travels, the symptoms that look exactly like software bugs, and the three wiring practices that fix most of it: single-point grounding, twisted-pair wiring, and keeping signal cables away from motor cables.

---

## Quick reference

The condensed version of the comparison matrix. The full table, with isolation type, switching speed and current capability, is in Section 1.6 of the report.

| | Optocoupler | Relay | Solid-State Relay |
|---|---|---|---|
| Primary job | Transfer signals | Switch power | Switch power |
| Switches electrical power | No | Yes | Yes |
| Can drive a motor directly | No | Yes | Yes |
| PWM capable | No | No | DC yes, AC usually not |
| Moving parts | No | Yes | No |
| ESP32 direct drive | Usually, with a resistor | Usually needs a driver | Usually, within GPIO limits |

**Where each one belongs in an ESP32 and L298N build:**

| Circuit point | Carries | Use | Never use |
|---|---|---|---|
| IN1, IN2 | Logic signal | GPIO direct, or optocoupler if isolation is needed | Relay, SSR |
| ENA | PWM logic signal | GPIO direct | Relay, SSR |
| Battery to L298N | Power | Relay or SSR | Optocoupler alone |
| L298N to motor | High-current power | Handled internally by the H-bridge | Anything external |

The one question that resolves almost every case: **am I sending a command, or switching power?**

---

## Figures

All 15 diagrams were drawn specifically for this report using matplotlib. They are original schematic illustrations, not reproductions from the sources listed below, and they can be reused freely with attribution.

Included: the two electrical worlds, optocoupler internals, relay internals, SSR block diagram, the control and power layers of a motor system, device placement in the circuit, flyback diode wiring, the voltage spike waveform, the isolation barrier, shared rail vs. separate branches, the 3.3 V rail sag at motor start, conducted and radiated EMI paths, daisy-chain vs. star ground, twisted-pair field cancellation, and cable routing.

---

## References

**Source notes**
- [Day 13 working notes (Notion)](https://app.notion.com/p/Day-13-3b254ef921fa80ba8377cadb1190fd39?source=copy_link), the notes this report was written from.

**Section 1**
- [Omron, Relays: Basic Knowledge](https://components.omron.com/us-en/products/basic-knowledge/relays/basics)
- [Galco, Industrial Control: Relays](https://www.galco.com/resources/industrial-control/relays)
- [Electronics Tutorials, The Optocoupler](https://www.electronics-tutorials.ws/blog/optocoupler.html)
- [Build Electronic Circuits, Optocoupler](https://www.build-electronic-circuits.com/optocoupler/)
- [Electronics Tutorials, Solid State Relay](https://www.electronics-tutorials.ws/power/solid-state-relay.html)
- [GEYA, Optocoupler vs Solid State Relay](https://www.geya.net/optocoupler-vs-solid-state-relay/)
- [Shenler, Solid State Relay vs Mechanical Relay](https://www.shenler.com/new/solid-state-relay-vs-mechanical/)

**Section 2**
- [CircuitBread, How Does a Flyback Diode Work?](https://www.circuitbread.com/ee-faq/how-does-a-flyback-diode-work)
- [Allelco, What Is a Flyback Diode](https://www.allelcoelec.com/blog/What-Is-a-Flyback-Diode-How-It-Works-and-Why-Your-Circuit-Needs-One.html)
- [Flux, Understanding the Flyback Diode](https://www.flux.ai/p/blog/understanding-the-flyback-diode-your-essential-guide-to-functionality-and-why-you-need-one)

**Section 3**
- [GeeksforGeeks, Electromagnetic Interference](https://www.geeksforgeeks.org/electronics-engineering/electromagnetic-interference/)
- [TME, EMI Identification and Prevention Methods in Electronics](https://www.tme.eu/en/news/library-articles/page/68660/electromagnetic-interference-emi-identification-and-prevention-methods-in-electronics/)

---

## A note on the numbers

The worked examples use typical values drawn from the training material rather than measurements from a specific build: PWM at roughly 5 kHz, an ESP32 drawing 80 to 250 mA with Wi-Fi peaks to 500 mA, a small motor drawing 500 mA nominal and 2 to 5 A stalled, and a 0.1 Ω wiring resistance producing a 0.3 V drop at 3 A. Check them against the datasheets for your own hardware before designing around them.

---

## Author

**Malak Alsharqawi**, Cyber Robot, IoT Training Program, 2026.
