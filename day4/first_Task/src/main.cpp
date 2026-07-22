

#include <Arduino.h>

// --- Pin Definitions ---
const int SENSOR_PIN     = 4;   // ACS712 Signal Output (ADC1)
const int RELAY_PIN      = 5;   // Relay Control Input (Active-LOW)
const int ESTOP_PIN      = 6;   // Multi-function Push Button (E-Stop / Reset)

const int LED_RUNNING    = 15;  // Green LED: Motor Running
const int LED_STOPPED    = 7;   // Red LED: Motor Stopped / Fault Shutdown
const int LED_LOW_CURR   = 17;  // LED: Normal Low Current Draw
const int LED_HIGH_CURR  = 16;  // Yellow LED: High Current Fault Indicator

// --- Sensor Calibration Parameters ---
const float SENSITIVITY       = 0.1221; // Scaled sensitivity for 3.3V power (122.1 mV/A)
const float RESTING_OFFSET    = 1.572;  // Your measured 0A baseline voltage
const float OVERCURRENT_LIMIT = 0.3; // Threshold to trip relay (0.43)

// --- Active-LOW Relay Defines ---
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// --- System Flags & Button Tracking ---
bool eStopTriggered   = false;
bool overcurrentFault = false;

int lastBtnState      = HIGH;   // Edge detection for button
float trippedCurrent  = 0.0;    // Holds current value at time of fault

// Centralized helper function to control LED state combinations
void updateLEDs(bool running, bool stopped, bool lowCurr, bool highCurr) {
  digitalWrite(LED_RUNNING,   running  ? HIGH : LOW);
  digitalWrite(LED_STOPPED,   stopped  ? HIGH : LOW);
  digitalWrite(LED_LOW_CURR,  lowCurr  ? HIGH : LOW);
  digitalWrite(LED_HIGH_CURR, highCurr ? HIGH : LOW);
}

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000); // Wait for USB CDC connection

  // 1. Configure Relay Pin
  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); // Safety init: Motor OFF

  // 2. Configure LED Pins
  pinMode(LED_RUNNING, OUTPUT);
  pinMode(LED_STOPPED, OUTPUT);
  pinMode(LED_LOW_CURR, OUTPUT);
  pinMode(LED_HIGH_CURR, OUTPUT);

  // Initial State: Stopped (Red) ON
  updateLEDs(false, true, false, false);

  // 3. Configure Button Pin
  pinMode(ESTOP_PIN, INPUT_PULLUP);

  // 4. Configure ADC Attenuation
  analogSetPinAttenuation(SENSOR_PIN, ADC_11db);

  Serial.println("\n=============================================");
  Serial.println("   MOTOR PROTECTION SYSTEM READY             ");
  Serial.println("=============================================");
  Serial.print("Overcurrent Trip Limit: ");
  Serial.print(OVERCURRENT_LIMIT, 3);
  Serial.println(" A");
  Serial.println("Yellow LED (GPIO 16) indicates High Current Fault.\n---------------------------------------------\n");
}

void loop() {
  // -------------------------------------------------------------
  // 🔘 1. READ PUSH BUTTON (PRESS TO RESTART / E-STOP)
  // -------------------------------------------------------------
  int btnState = digitalRead(ESTOP_PIN);

  // Transition check from HIGH to LOW (Button Pressed)
  if (btnState == LOW && lastBtnState == HIGH) {
    delay(50); // 50ms Debounce delay

    if (eStopTriggered || overcurrentFault) {
      // --- RESET ACTION ---
      eStopTriggered   = false;
      overcurrentFault = false;
      trippedCurrent   = 0.0;

      // Turn motor ON & set LEDs to Running state
      digitalWrite(RELAY_PIN, RELAY_ON);
      updateLEDs(true, false, true, false); 

      Serial.println("\n🔄 BUTTON PRESSED: System Reset! Restarting Motor...");
    } else {
      // --- EMERGENCY STOP ACTION ---
      eStopTriggered = true;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      updateLEDs(false, true, false, false); // Turn Stopped (Red) LED ON

      Serial.println("\n🚨 BUTTON PRESSED: Emergency Stop Activated!");
    }
  }
  lastBtnState = btnState; // Save current state for next loop pass

  // -------------------------------------------------------------
  // 🚨 2. EMERGENCY STOP LOCKOUT STATE
  // -------------------------------------------------------------
  if (eStopTriggered) {
    digitalWrite(RELAY_PIN, RELAY_OFF);
    updateLEDs(false, true, false, false); // Red (Stopped) ON
    
    Serial.println("🚨 FAULT LOCKOUT: E-Stop Active! Press Button to RESET System.");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // ⚠️ 3. OVERCURRENT FAULT LOCKOUT STATE
  // -------------------------------------------------------------
  if (overcurrentFault) {
    digitalWrite(RELAY_PIN, RELAY_OFF); // Cut motor power immediately
    
    // LEDs: Stopped (Red) ON, Yellow LED (High Current GPIO 16) ON
    updateLEDs(false, true, false, true);

    Serial.print("⚠️ OVERCURRENT FAULT: Motor Stopped! Tripped at ");
    Serial.print(trippedCurrent, 3);
    Serial.println(" A | Yellow LED ON | Press Button to RESET.");
    delay(500);
    return;
  }

  // -------------------------------------------------------------
  // 🟢 4. NORMAL RUNNING OPERATION & CURRENT SENSING
  // -------------------------------------------------------------
  digitalWrite(RELAY_PIN, RELAY_ON); // Keep motor running

  // Read and average 50 ADC samples
  long sum = 0;
  for (int i = 0; i < 50; i++) {
    sum += analogRead(SENSOR_PIN);
    delay(2);

    // Fast E-Stop check inside sampling loop
    if (digitalRead(ESTOP_PIN) == LOW && lastBtnState == HIGH) {
      eStopTriggered = true;
      digitalWrite(RELAY_PIN, RELAY_OFF);
      updateLEDs(false, true, false, false);
      lastBtnState = LOW;
      return;
    }
  }

  float avgADC = sum / 50.0;
  float voltage = (avgADC / 4095.0) * 3.3;
  float current = abs((voltage - RESTING_OFFSET) / SENSITIVITY);

  // -------------------------------------------------------------
  // 🔍 5. EVALUATE THRESHOLD & TRIGGER PROTECTION
  // -------------------------------------------------------------
  if (current >= OVERCURRENT_LIMIT) {
    overcurrentFault = true;
    trippedCurrent   = current;
    
    // INSTANT SAFETY ACTION: Cut Relay & turn ON Yellow High-Current LED
    digitalWrite(RELAY_PIN, RELAY_OFF);
    updateLEDs(false, true, false, true); // Red (7) ON, Yellow (16) ON
    
    Serial.print("\n🚨 OVERCURRENT TRIP DETECTED! Current reached: ");
    Serial.print(trippedCurrent, 3);
    Serial.println(" A. Motor stopped & Yellow LED Lit.");
  } else {
    // Normal Safe Operation: Green (15) ON, Low Current LED (17) ON
    updateLEDs(true, false, true, false);

    Serial.print("Pin Volts: ");
    Serial.print(voltage, 3);
    Serial.print(" V | Measured Current: ");
    Serial.print(current, 3);
    Serial.println(" A [OK]");
  }

  delay(200);
}