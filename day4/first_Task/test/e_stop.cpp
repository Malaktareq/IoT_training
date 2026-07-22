/*
 * Integrated Motor Protection Subsystem (Polling Mode)
 * Components:
 * - HW-872A Current Sensor (GPIO 4)
 * - SRD-05VDC Relay Module (GPIO 5)
 * - E-Stop Pushbutton (GPIO 6 - Polling Mode)
 */

#include <Arduino.h>

// --- Pin Definitions ---
const int SENSOR_PIN = 4;  // ACS712 Signal Output (ADC1)
const int RELAY_PIN  = 5;  // Relay Control Input (Active-LOW)
const int ESTOP_PIN  = 6;  // E-Stop Push Button (Pulled up to 3.3V)

// --- Sensor Calibration & Safety Limits ---
const float SENSITIVITY       = 0.185;  // 0.185 V/A for 5A ACS712 model
const float RESTING_OFFSET    = 1.572;  // Measured 0A baseline voltage at 3.3V
const float OVERCURRENT_LIMIT = 0.350;  // Cutoff threshold (0.350 A / 350 mA)

// --- Active-LOW Relay Defines ---
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- System State Flags ---
bool eStopTriggered   = false;
bool overcurrentFault = false;

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait for native USB CDC Serial

  // 1. Configure Relay Output Pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); // Safe boot: Motor OFF

  // 2. Configure E-Stop Button Pin with Internal Pullup
  pinMode(ESTOP_PIN, INPUT_PULLUP);

  // 3. Configure ADC for Current Sensor
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

  Serial.println("\n=============================================");
  Serial.println("   INTEGRATED SYSTEM READY (POLLING MODE)    ");
  Serial.println("=============================================");
  Serial.print("Calibrated 0A Baseline: ");
  Serial.print(RESTING_OFFSET, 3);
  Serial.println(" V");
  Serial.print("Overcurrent Cutoff Limit: ");
  Serial.print(OVERCURRENT_LIMIT, 3);
  Serial.println(" A\n---------------------------------------------\n");
}

void loop() {
  // -------------------------------------------------------------
  // 🔘 1. READ PUSH BUTTON STATE DIRECTLY
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
    digitalWrite(RELAY_PIN, RELAY_OFF); // Guarantee motor remains disconnected
    Serial.println("🚨 CRITICAL FAULT: E-Stop Activated! | Button State: LOW (0) | Relay LOCKED OFF");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // ⚠️ 3. CHECK OVERCURRENT FAULT LOCKOUT
  // -------------------------------------------------------------
  if (overcurrentFault) {
    digitalWrite(RELAY_PIN, RELAY_OFF); // Guarantee motor remains disconnected
    Serial.print("⚠️ OVERCURRENT FAULT! | Button State: ");
    Serial.print(btnState == HIGH ? "HIGH (1)" : "LOW (0)");
    Serial.println(" | Relay LOCKED OFF");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // 🟢 4. NORMAL OPERATION & CURRENT SENSING
  // -------------------------------------------------------------
  digitalWrite(RELAY_PIN, RELAY_ON); // Power the motor

  // Read and average 50 ADC samples to smooth signal noise
  long rawSum = 0;
  for (int i = 0; i < 50; i++) {
    rawSum += analogRead(SENSOR_PIN);
    delay(1);
    
    // Check button status mid-sampling for faster shutdown response
    if (digitalRead(ESTOP_PIN) == LOW) {
      eStopTriggered = true;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      return;
    }
  }

  float avgADC = rawSum / 50.0;
  float pinVoltage = (avgADC / 4095.0) * 3.3;
  float measuredCurrent = (pinVoltage - RESTING_OFFSET) / SENSITIVITY;

  // Clamp small floating noise near 0A rest
  if (measuredCurrent < 0.0) measuredCurrent = 0.0;

  // Output Telemetry with Button State
  Serial.print("Btn State: ");
  Serial.print(btnState == HIGH ? "HIGH (1) [IDLE]  " : "LOW (0) [PRESSED]");
  Serial.print(" | Volts: ");
  Serial.print(pinVoltage, 3);
  Serial.print(" V | Current: ");
  Serial.print(measuredCurrent, 3);
  Serial.println(" A");

  // -------------------------------------------------------------
  // 🔍 5. OVERCURRENT CHECK
  // -------------------------------------------------------------
  if (measuredCurrent >= OVERCURRENT_LIMIT) {
    overcurrentFault = true;
    digitalWrite(RELAY_PIN, RELAY_OFF); // Cut power immediately
    Serial.println("\n🚨 OVERCURRENT TRIP DETECTED!");
  }

  delay(200); // Response loop refresh rate
}