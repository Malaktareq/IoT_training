# IoT Training Program — Day 1

**Prepared by:** Malak Alsharqawi
**Organization:** Cyber Robot
**Date:** 19/07/2026

Embedded systems training log for setting up an ESP32 development environment on Windows via WSL, and version-controlling the project on GitHub over SSH.

---

## Summary

This session set up a complete ESP32 development environment by linking Windows hardware to a Linux-based WSL backend for building and flashing firmware. The environment was verified end-to-end by running an external LED blink sketch, and a secure, password-free connection to GitHub was established using SSH.

**Tasks completed:**
- ✅ VS Code & PlatformIO Setup + ESP32 Blink
- ✅ Git & GitHub (SSH)

---

## Task 1 — VS Code & PlatformIO Setup + ESP32 Blink

**Objective:** Set up a complete embedded development environment capable of recognizing, building for, and flashing firmware to an ESP32 microcontroller from a Linux-based toolchain running on Windows.

### Procedure

1. **Install the CP210x USB-to-UART driver** — so Windows recognizes the ESP32 over USB.
2. **Install WSL**
   - Ran the one-command install and restarted to apply virtualization layers.
   - Initialized the Linux user account on first boot.
   - Verified the installed distro/version with the status list command.
   - Updated the package database before installing compilers/extensions.
3. **Install and configure PlatformIO**
   - Installed the PlatformIO IDE extension in VS Code, connected to the WSL: Ubuntu backend.
   - Fixed a Python interpreter error by installing `python3` and `python3-venv` inside WSL, then retried setup.
4. **Configure USB/serial passthrough (usbipd)**
   - Granted the Linux user serial device access (`dialout` group).
   - Force-bound the ESP32's USB Bus ID on the Windows host.
   - Attached the device to WSL with `--auto-attach` for persistent reconnection.

### Setup Commands

```bash
# Install WSL
wsl --install

# Verify installation
wsl --list -v

# Update Linux packages
sudo apt update && sudo apt upgrade -y

# Fix PlatformIO Python interpreter error
sudo apt update && sudo apt install python3 python3-venv -y

# Grant serial device permissions
sudo usermod -a -G dialout $USER

# Bind and attach ESP32 (run in Windows PowerShell as Administrator)
usbipd list
usbipd bind --busid <YOUR_BUS_ID> --force
usbipd attach --wsl --busid <YOUR_BUS_ID> --auto-attach
```

### Firmware — External LED Blink (GPIO 18, 115200 baud)

```cpp
#include <Arduino.h>

#define EXTERNAL_LED 18

void setup() {
  Serial.begin(115200);
  pinMode(EXTERNAL_LED, OUTPUT);
}

void loop() {
  digitalWrite(EXTERNAL_LED, HIGH);
  delay(1000);
  Serial.println("External LED is ON");
  digitalWrite(EXTERNAL_LED, LOW);
  Serial.println("External LED is OFF");
  delay(1000);
}
```

### Issues & Troubleshooting

PlatformIO initially failed with **"Can not find working Python 3.6+ Interpreter"** because it was searching for Python inside the WSL Linux environment rather than Windows. Resolved by installing `python3` and `python3-venv` inside WSL and retrying.

### Results

The ESP32 was successfully recognized by WSL over a shared USB/serial connection. PlatformIO built and uploaded the Blink sketch, and the serial monitor confirmed `"External LED is ON/OFF"` output at 1-second intervals.

---

## Task 2 — Git & GitHub (SSH)

**Objective:** Turn an existing local project folder into a version-controlled Git repository and link it to a remote GitHub repository using SSH.

### Procedure

1. **SSH key setup** — generated a new `ed25519` key pair and added the public key to GitHub (Settings → SSH and GPG keys).
2. **Initialize and link the repository** — ran `git init`, staged/committed existing files, renamed the branch to `main`, and added the GitHub remote via SSH.
3. **Push to GitHub** — pushed local commit history and set upstream tracking; accepted GitHub's host fingerprint on first connection.

### Commands

```bash
# Generate SSH key
ssh-keygen -t ed25519 -C "your_email@example.com"
cat ~/.ssh/id_ed25519.pub   # copy this into GitHub

# Initialize and commit
cd ~/test/IoT_training
git init
git add .
git commit -m "Initial commit of existing project files"
git branch -M main
git remote add origin git@github.com:<user>/<repo>.git

# Push
git push -u origin main
```

### Issues & Troubleshooting

None — the first-time SSH host verification prompt was accepted as expected.

### Results

The local project was successfully version-controlled and pushed to the remote GitHub repository over SSH; subsequent commits can now be pushed directly with `git push`.

---

## Hardware

### Schematic

External LED wired to **GPIO 18**, designed in Cirkit Designer — ESP32 board with the LED and ground/signal lines connected as shown in the full technical report.

### GPIO Pinout (ESP32-WROOM-32)

| Left Side (Input Only where noted) | Right Side |
|---|---|
| GPIO36 *(Input Only)* | GPIO23 |
| GPIO39 *(Input Only)* | GPIO22 |
| GPIO34 *(Input Only)* | GPIO1 |
| GPIO35 *(Input Only)* | GPIO3 |
| GPIO32 | GPIO21 |
| GPIO33 | GPIO19 |
| GPIO25 | GPIO18 |
| GPIO26 | GPIO5 |
| GPIO27 | GPIO17 |
| GPIO14 | GPIO16 |
| GPIO12 | GPIO4 |
| GPIO13 | GPIO2 |
| | GPIO15 |

> GPIO18 drives the external LED used in the Task 1 blink sketch.

---

## Project Documentation & Notes

- 📓 **Development notes & key learnings:** [First-Day Notion Notebook](https://app.notion.com/p/Day-1-3a254ef921fa80769a0dc1db6c03625e) — daily progression logs, troubleshooting steps, and core takeaways.
- 🔌 **Hardware schematic & circuit layout:** [Open Circuit Designer Workspace](https://app.cirkitdesigner.com/project/48838bfc-17e2-4cde-9776-73d769509f12) — system architecture, GPIO pinout mapping, and electrical connections.

> Replace the `#` links above with the actual Notion and Circuit Designer URLs from the source report before publishing.

---


*End of Day 1 — IoT Training Program, Cyber Robot*
