/*
 * Blink External LED
 * Turns on an external LED on GPIO 18 for one second,
 * then off for one second, repeatedly.
 */

#include <Arduino.h>

// Explicitly define your external LED pin here
#define EXTERNAL_LED 18

void setup()
{
  Serial.begin(115200);
  // Initialize GPIO 18 as an output.
  pinMode(EXTERNAL_LED, OUTPUT);
}

void loop()
{
  // Turn the external LED on (HIGH voltage)
  digitalWrite(EXTERNAL_LED, HIGH);
  delay(1000);
  Serial.println("External LED is ON");
  // Turn the external LED off (LOW voltage)
  digitalWrite(EXTERNAL_LED, LOW);
  Serial.println("External LED is OFF");
  delay(1000);
}