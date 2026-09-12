# IoT and Embedded Systems Training — Cyber Robot

This repository documents the practical work, code, reports, circuit diagrams, and project results completed during my **eight-week university field training** in the **IoT Department at Cyber Robot (Smart Industrial Automation)**.

- **Location:** Amman, Jordan
- **Training period:** 19 July 2026 – 10 September 2026
- **Duration:** 8 weeks
- **Main platforms:** ESP32, PlatformIO, Arduino framework, ESP-IDF, FreeRTOS, Node-RED, MQTT, Firebase, and CNC systems

## Training Overview

The training combined embedded programming, electronics, networking, IoT communication, real-time systems, and industrial CNC applications. The work progressed from basic ESP32 exercises to connected safety systems, dashboards, cloud data logging, RTOS applications, and a final smart CNC monitoring project.

## Work Completed by Week

| Week | Main Work Completed |
|---|---|
| **Week 1** | Set up the ESP32 development environment using VS Code, PlatformIO, Git, and GitHub. Practised GPIO, digital and analog input/output, IR sensors, LEDs, emergency-stop logic, ADC measurements, current sensing, L298N motor control, and basic C++ object-oriented programming. |
| **Week 2** | Studied ESP32 storage and communication concepts, including SPIFFS, Preferences, Wi-Fi Station and Access Point modes, BME280 sensor integration, a local ESP32 web server, and an ESP-NOW safety interlock between two ESP32 boards with a communication timeout. |
| **Week 3** | Implemented MQTT communication using ESP32 devices and public and local brokers. Compared MQTT, HTTP, TCP/IP, and ESP-NOW; studied IP addressing, NAT, TLS, and networking fundamentals; and built bidirectional motor-control and telemetry applications. |
| **Week 4** | Configured a local Mosquitto broker using Windows and WSL2, connected ESP32 devices to it, and developed a Node-RED dashboard for monitoring sensor readings and controlling motor power, direction, and speed. Safety and reconnect behaviour were also tested. |
| **Week 5** | Extended the IoT dashboard by logging live and historical data to Firebase Realtime Database. Started working with ESP-IDF and continued practical electronics and CNC sessions, including CNC components, stepper drivers, limit switches, homing, wiring, safety, and pen-plotter design. |
| **Week 6** | Studied FreeRTOS on ESP32 and separated sensor acquisition and output control into independent tasks. Practised priorities, delays, shared data, logging, timing behaviour, and system responsiveness. CNC testing and troubleshooting also continued. |
| **Week 7** | Replaced shared global data with FreeRTOS queues and compared different queue depths and overflow behaviour. Continued the CNC application work and prepared the architecture, sensing, safety, and communication requirements for the final project. |
| **Week 8** | Developed the final **Smart CNC Monitoring and Safety System** using ESP32 devices, sensors, MQTT, and a Node-RED dashboard. The system focused on remote CNC status monitoring, environmental and vibration measurements, emergency-stop behaviour, and local-network communication. The final week also included a **factory visit** to observe industrial machines, automation systems, and real production processes. |

> Some tasks continued across more than one week. The folders are organized according to the day on which code, testing, or documentation was added.

## Main Practical Projects

### ESP32 Sensor and Motor Control

- Read digital IR sensors and analog current measurements.
- Used a BME280 for temperature, humidity, and pressure.
- Controlled DC motors through an L298N driver.
- Added emergency-stop and overcurrent safety logic.
- Applied non-blocking timing and sensor-value smoothing.

### ESP-NOW Safety Interlock

Two ESP32 boards exchanged IR-detection states directly through ESP-NOW. Each device controlled its partner motor and stopped safely if communication was lost for more than two seconds.

### MQTT and Node-RED System

- Connected ESP32 devices to public and local MQTT brokers.
- Published sensor readings and motor status.
- Subscribed to dashboard commands for motor power, direction, and speed.
- Built a Node-RED dashboard for live monitoring and control.
- Added reconnect logic and a fail-safe motor stop after communication loss.

### Firebase Data Logging

The Node-RED flow was extended to store:

- Current sensor and motor state.
- Timestamped historical readings.
- Data from the same validated values displayed on the local dashboard.

### FreeRTOS Applications

- Created separate sensor and output tasks.
- Used task priorities and blocking delays.
- Replaced shared global variables with FreeRTOS queues.
- Compared queue depth 1 and queue depth 5.
- Logged send, receive, timeout, and overflow behaviour.

### CNC and Pen-Plotter Sessions

The CNC sessions covered both the mechanical and electrical sides of a CNC system:

- Ruida RDC6445S controller.
- 3DM580S stepper drivers and STEP/DIR control.
- X/Y stepper motors, belts, pulleys, and motion mechanisms.
- Limit switches, homing, emergency stops, and safe wiring.
- Power supplies, relays, optocouplers, grounding, and EMI reduction.
- Pen-lifting mechanism using a solenoid and spring holder.
- CNC signal testing and troubleshooting.

### Final Project — Smart CNC Monitoring and Safety System

The final project combined the main subjects studied during the training. ESP32 devices collected machine and environmental data and sent it independently to a Node-RED dashboard through MQTT. The project included:

- CNC running and status monitoring.
- Temperature, humidity, and pressure sensing.
- MPU6050 vibration and motion measurement.
- Emergency-stop monitoring and safety latching.
- Local-network MQTT communication.
- Dashboard visualization and system-status reporting.

## Repository Structure

The repository is arranged chronologically by week and training day:

```text
IoT_training/
├── week1/
├── week2/
├── week3/
├── week4/
├── week5/
├── week6/
├── week7/
├── week8/
└── Refrences/
```

Depending on the activity, each day may contain:

- ESP32 firmware or ESP-IDF source code.
- PlatformIO configuration.
- Technical reports in Word or PDF format.
- Circuit diagrams, pinouts, photographs, or videos.
- Node-RED flows and Firebase data exports.
- A README explaining the task and results.

## Technologies and Concepts

- **Embedded development:** ESP32, Arduino framework, ESP-IDF, PlatformIO, C/C++.
- **Sensors and actuators:** BME280, MPU6050, IR sensors, current sensor, DC motors, stepper motors, relays, and solenoids.
- **Communication:** Wi-Fi, ESP-NOW, MQTT, HTTP, TCP/IP, and TLS.
- **IoT tools:** Mosquitto, EMQX, Node-RED, MQTTX, and Firebase Realtime Database.
- **Real-time systems:** FreeRTOS tasks, priorities, delays, queues, timeouts, and logging.
- **Industrial systems:** CNC controllers, stepper drivers, homing, limit switches, electrical isolation, grounding, and safety interlocks.

## Security Note

Wi-Fi credentials, passwords, and local network details are intentionally excluded from this repository. Placeholder values must be replaced locally before building or uploading a project.
