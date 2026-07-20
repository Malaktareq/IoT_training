#include <Arduino.h>

// put function declarations here
#define IR_PIN 18
#define LED_PIN 19
void setup() {
  // put your setup code here, to run once:
  pinMode(IR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  Serial.begin(115200);
}

void loop() {
  int irValue = digitalRead(IR_PIN);
  if (irValue == LOW) {
    digitalWrite(LED_PIN, HIGH);
    Serial.println("IR sensor detected an object!");
    Serial.print("IR value: ");
    Serial.println(irValue);
  } else {
    digitalWrite(LED_PIN, LOW);
  }
  delay(100); // Delay for stability
}