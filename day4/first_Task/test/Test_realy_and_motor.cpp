#include <Arduino.h>

const int RELAY_PIN = 5; // GPIO 5 connected to Relay IN

// Active-LOW configuration:
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000);

  pinMode(RELAY_PIN, OUTPUT);
  digitalWrite(RELAY_PIN, RELAY_OFF); // Ensure motor starts OFF

  Serial.println("\n--- Testing SRD-05VDC Relay Control ---");
}

void loop() {
  Serial.println("Relay ON -> Motor Running...");
  digitalWrite(RELAY_PIN, RELAY_ON);
  delay(2000);

  Serial.println("Relay OFF -> Motor Stopped.");
  digitalWrite(RELAY_PIN, RELAY_OFF);
  delay(2000);
}