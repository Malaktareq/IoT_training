/*
 * Complete Motor Protection System
 * Microcontroller: ESP32-S3
 *
 * Configured Settings:
 * - Current Sensor: HW-872A on GPIO 4 (Baseline: 1.572 V, Sensitivity: 0.185 V/A)
 * - Relay Control:  SRD-05VDC on GPIO 5 (Active-LOW)
 * - E-Stop Button:  Push Button on GPIO 6 (INPUT_PULLUP)
 * - Green LED:      Motor Running (GPIO 15)
 * - Red LED:        Motor Stopped / Shutdown (GPIO 7)
 * - Low Current:    Normal Current Indicator (GPIO 17)
 * - High Current:   Overcurrent Fault Indicator (GPIO 16)
 */

#include <Arduino.h>

// --- Pin Definitions ---
const int SENSOR_PIN     = 4;   // ACS712 Signal Output
const int RELAY_PIN      = 5;   // Relay Control Input (Active-LOW)
const int ESTOP_PIN      = 6;   // E-Stop Push Button

const int LED_RUNNING    = 15;  // Green LED: Motor Running
const int LED_STOPPED    = 7;   // Red LED: Motor Stopped / Fault Shutdown
const int LED_LOW_CURR   = 17;  // LED: Normal Low Current Draw
const int LED_HIGH_CURR  = 16;  // LED: High Current / Overcurrent Fault

// --- Sensor Calibration Parameters ---
const float SENSITIVITY       = 0.185;  // 0.185 V/A (185 mV/A) for 5A ACS712 model
const float RESTING_OFFSET    = 1.572;  // Your measured zero-current baseline voltage
const float OVERCURRENT_LIMIT =60.0;  // Cutoff limit: 0.350 A (350 mA)

// --- Active-LOW Relay Defines ---
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- System State Flags ---
bool eStopTriggered   = false;
bool overcurrentFault = false;

// Centralized helper function to manage LED state combinations
void updateLEDs(bool running, bool stopped, bool lowCurr, bool highCurr) {
  digitalWrite(LED_RUNNING,   running  ? HIGH : LOW);
  digitalWrite(LED_STOPPED,   stopped  ? HIGH : LOW);
  digitalWrite(LED_LOW_CURR,  lowCurr  ? HIGH : LOW);
  digitalWrite(LED_HIGH_CURR, highCurr ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait for native USB CDC Serial connection

  // 1. Configure Relay Output
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); // Safety init: Motor OFF

  // 2. Configure LED Pins
  pinMode(LED_RUNNING, OUTPUT);
  pinMode(LED_STOPPED, OUTPUT);
  pinMode(LED_LOW_CURR, OUTPUT);
  pinMode(LED_HIGH_CURR, OUTPUT);

  // Initial Boot LED State: Stopped (Red) ON, all others OFF
  updateLEDs(false, true, false, false);

  // 3. Configure E-Stop Button Pin with Internal Pull-Up
  pinMode(ESTOP_PIN, INPUT_PULLUP);

  // 4. Configure ADC Attenuation for 0 - 3.1V range on ESP32-S3
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

  Serial.println("\n=============================================");
  Serial.println("   COMPLETE MOTOR PROTECTION SYSTEM READY    ");
  Serial.println("=============================================");
  Serial.print("Zero-Current Baseline Set To: ");
  Serial.print(RESTING_OFFSET, 3);
  Serial.println(" V");
  Serial.print("Overcurrent Cutoff Threshold: ");
  Serial.print(OVERCURRENT_LIMIT, 3);
  Serial.println(" A\n---------------------------------------------\n");
}

void loop() {
  // -------------------------------------------------------------
  // 🔘 1. READ E-STOP PUSH BUTTON
  // -------------------------------------------------------------
  int btnState = digitalRead(ESTOP_PIN);

  // Check if button is actively pressed (LOW = connected to GND)
  if (btnState == LOW) {
    eStopTriggered = true;
  }

  // -------------------------------------------------------------
  // 🚨 2. CHECK EMERGENCY STOP LOCKOUT
  // -------------------------------------------------------------
  if (eStopTriggered) {
    digitalWrite(RELAY_PIN, RELAY_OFF); // Keep motor power isolated
    updateLEDs(false, true, false, false); // Green OFF, Red (Stopped) ON
    
    Serial.println("🚨 CRITICAL FAULT: E-Stop Activated! | Relay LOCKED OFF");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // ⚠️ 3. CHECK OVERCURRENT FAULT LOCKOUT
  // -------------------------------------------------------------
  if (overcurrentFault) {
    digitalWrite(RELAY_PIN, RELAY_OFF); // Keep motor power isolated
    updateLEDs(false, true, false, true); // Red (Stopped) ON, High Current LED (16) ON

    Serial.println("⚠️ OVERCURRENT FAULT: Safety Shutdown Active! | Relay LOCKED OFF");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // 🟢 4. NORMAL RUNNING OPERATION & CURRENT SENSING
  // -------------------------------------------------------------
  digitalWrite(RELAY_PIN, RELAY_ON); // Energize relay to power motor

  // Average 50 samples to filter out ADC noise
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(2);

    // Fast response: Check button state during sampling loop
    if (digitalRead(ESTOP_PIN) == LOW) {
      eStopTriggered = true;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      updateLEDs(false, true, false, false);
      return;
    }
  }

  float avgADC = sum / 50.0;
  
  // Convert 12-bit ADC reading (0 to 4095) to pin voltage (0 to 3.3V)
  float voltage = (avgADC / 4095.0) * 3.3;

  // Calculate current relative to your 1.572V baseline
float current = abs((voltage - RESTING_OFFSET) / SENSITIVITY);

  // Clamp small floating noise near 0A rest
 // if (current < 0.0) current = 0.0;

  // -------------------------------------------------------------
  // 🔍 5. EVALUATE CURRENT & UPDATE INDICATORS
  // -------------------------------------------------------------
  if (current >= OVERCURRENT_LIMIT) {
    overcurrentFault = true;
    digitalWrite(RELAY_PIN, RELAY_OFF); // Trip relay immediately
    updateLEDs(false, true, false, true); // Stopped (Red) ON, High Current (16) ON
    Serial.println("\n🚨 OVERCURRENT TRIP DETECTED! Exceeded safety limit.");
  } else {
    // Normal Safe Operation: Running (Green 15) ON, Low Current (17) ON
    updateLEDs(true, false, true, false);

    Serial.print("Raw ADC: ");
    Serial.print(avgADC, 1);
    Serial.print(" | Pin Voltage: ");
    Serial.print(voltage, 3);
    Serial.print(" V | Measured Current: ");
    Serial.print(current, 3);
    Serial.println(" A [OK]");
  }

  delay(200); // Response loop refresh rate
}