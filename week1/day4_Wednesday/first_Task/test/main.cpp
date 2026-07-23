/*
 * Integrated Motor Protection System with Instant Reset & LED Response
 * Hardware: ESP32-S3
 */

#include <Arduino.h>

// --- Pin Definitions ---
const int SENSOR_PIN     = 4;   // ACS712 Signal Output
const int RELAY_PIN      = 5;   // Relay Control Input (Active-LOW)
const int ESTOP_PIN      = 6;   // Multi-function Push Button (E-Stop / Reset)

const int LED_RUNNING    = 15;  // Green LED: Motor Running
const int LED_STOPPED    = 7;   // Red LED: Motor Stopped / Shutdown
const int LED_LOW_CURR   = 17;  // Yellow/Blue LED: Normal Current
const int LED_HIGH_CURR  = 16;  // Red LED: Overcurrent Fault

// --- Calibration & Thresholds ---
const float SENSITIVITY       = 0.066;  // 185 mV/A for 5A ACS712
const float RESTING_OFFSET    = 1.600;  // Calibrated 0A baseline voltage at 3.3V
const float OVERCURRENT_LIMIT = 0.40;   // Cutoff limit: 0.40 A (400 mA)

// --- Active-LOW Relay Defines ---
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- System State Flags ---
bool eStopTriggered   = false;
bool overcurrentFault = false;

// Variables to track button state and last recorded current
int lastBtnState      = HIGH;
float trippedCurrent  = 0.0;

// Helper Function: Centralized LED Controller
void updateLEDs(bool running, bool stopped, bool lowCurr, bool highCurr) {
  digitalWrite(LED_RUNNING,   running  ? HIGH : LOW);
  digitalWrite(LED_STOPPED,   stopped  ? HIGH : LOW);
  digitalWrite(LED_LOW_CURR,  lowCurr  ? HIGH : LOW);
  digitalWrite(LED_HIGH_CURR, highCurr ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait for USB Serial CDC

  // Configure Output Pins
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); // Safety init: Motor OFF

  pinMode(LED_RUNNING, OUTPUT);
  pinMode(LED_STOPPED, OUTPUT);
  pinMode(LED_LOW_CURR, OUTPUT);
  pinMode(LED_HIGH_CURR, OUTPUT);

  // Initial Boot LED State: Stopped (Red) ON
  updateLEDs(false, true, false, false);

  // Configure E-Stop Button Pin with Internal Pull-Up
  pinMode(ESTOP_PIN, INPUT_PULLUP);

  // Configure ADC Attenuation
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

  Serial.println("\n=============================================");
  Serial.println("   MOTOR PROTECTION SYSTEM WITH RESET FEATURE");
  Serial.println("=============================================");
}

void loop() {
  // -------------------------------------------------------------
  // 🔘 1. READ PUSH BUTTON WITH EDGE DETECTION
  // -------------------------------------------------------------
  int btnState = digitalRead(ESTOP_PIN);

  // Detect new button press event (Transition from HIGH to LOW)
  if (btnState == LOW && lastBtnState == HIGH) {
    delay(50); // 50ms button debounce

    if (eStopTriggered || overcurrentFault) {
      // --- RESET ACTION ---
      eStopTriggered   = false;
      overcurrentFault = false;
      trippedCurrent   = 0.0;
      
      // INSTANTLY turn motor ON & set LEDs to Running state (No Lag!)
      digitalWrite(RELAY_PIN, RELAY_ON);
      updateLEDs(true, false, true, false); 

      Serial.println("\n🔄 BUTTON PRESSED: System Reset! Restarting Motor...");
    } else {
      // --- E-STOP ACTION ---
      eStopTriggered = true;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      updateLEDs(false, true, false, false); // INSTANTLY turn Stopped LED ON

      Serial.println("\n🚨 BUTTON PRESSED: Emergency Stop Activated!");
    }
  }
  lastBtnState = btnState; // Save state for next iteration

  // -------------------------------------------------------------
  // 🚨 2. EMERGENCY STOP LOCKOUT STATE
  // -------------------------------------------------------------
  if (eStopTriggered) {
    digitalWrite(RELAY_PIN, RELAY_OFF);
    updateLEDs(false, true, false, false); // Green OFF, Red (Stopped) ON
    
    Serial.println("🚨 FAULT LOCKOUT: E-Stop Active! Press Button to RESET.");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // ⚠️ 3. OVERCURRENT FAULT LOCKOUT STATE
  // -------------------------------------------------------------
  if (overcurrentFault) {
    digitalWrite(RELAY_PIN, RELAY_OFF);
    updateLEDs(false, true, false, true); // Red (Stopped) ON, High Current LED ON

    Serial.print("⚠️ OVERCURRENT FAULT! Tripped Current: ");
    Serial.print(trippedCurrent, 3);
    Serial.println(" A | Press Button to RESET.");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // 🟢 4. NORMAL RUNNING OPERATION & CURRENT SENSING
  // -------------------------------------------------------------
  digitalWrite(RELAY_PIN, RELAY_ON); // Power the motor via relay

  // Read and average 50 ADC samples
  long rawSum = 0;
  for (int i = 0; i < 50; i++) {
    rawSum += analogRead(SENSOR_PIN);
    delay(1);

    // Fast E-Stop check during sampling loop
    if (digitalRead(ESTOP_PIN) == LOW && lastBtnState == HIGH) {
      eStopTriggered = true;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      updateLEDs(false, true, false, false);
      lastBtnState = LOW;
      return;
    }
  }

  float avgADC = rawSum / 50.0;
  float pinVoltage = (avgADC / 4095.0) * 3.3;
  float measuredCurrent = abs((pinVoltage - RESTING_OFFSET) / SENSITIVITY);

  if (measuredCurrent < 0.0) measuredCurrent = 0.0; // Clamp floating noise

  // -------------------------------------------------------------
  // 🔍 5. EVALUATE OVERCURRENT THRESHOLD
  // -------------------------------------------------------------
  if (measuredCurrent >= OVERCURRENT_LIMIT) {
    overcurrentFault = true;
    trippedCurrent   = measuredCurrent;   // Store real value that caused trip
    digitalWrite(RELAY_PIN, RELAY_OFF);   // Trip relay immediately
    updateLEDs(false, true, false, true); // Stopped ON, High Current ON
    
    Serial.print("\n🚨 OVERCURRENT TRIP DETECTED! Current: ");
    Serial.print(trippedCurrent, 3);
    Serial.println(" A");
  } else {
    // Normal Safe Operation: Running (Green) ON, Low Current ON
    updateLEDs(true, false, true, false);

    Serial.print("Pin Volts: ");
    Serial.print(pinVoltage, 3);
    Serial.print(" V | Measured Current: ");
    Serial.print(measuredCurrent, 3);
    Serial.println(" A [OK]");
  }

  delay(200);
}