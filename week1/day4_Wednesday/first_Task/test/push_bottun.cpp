#include <Arduino.h>

const int BUTTON_PIN = 6;  // GPIO 6 -> Button
const int LED_PIN    = 7;  // GPIO 7 -> LED

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  pinMode(BUTTON_PIN, INPUT_PULLUP); // Keeps pin at 3.3V (HIGH) when open
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, LOW);        // LED off initially

  Serial.println("\n--- Push Button & LED Diagnostic Test ---");
}

void loop() {
  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == LOW) {
    // Button is pressed (connected to GND)
    digitalWrite(LED_PIN, HIGH);
    Serial.println("🟢 Button PRESSED -> State: LOW (0) | LED ON");
  } else {
    // Button is open (idle)
    digitalWrite(LED_PIN, LOW);
    Serial.println("⚪ Button IDLE -> State: HIGH (1) | LED OFF");
  }

  delay(200); // 0.2s refresh rate
}