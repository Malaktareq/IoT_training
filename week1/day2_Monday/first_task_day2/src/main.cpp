#include <Arduino.h>

// put function declarations here
#define IR_PIN 18
#define LED_PIN 19
#define POT_PIN 34

int flag = 1;
void setup() {
  // put your setup code here, to run once:
  pinMode(IR_PIN, INPUT);
  pinMode(LED_PIN, OUTPUT);
  pinMode(POT_PIN, INPUT);

  Serial.begin(115200);
  Serial.println("Checking IR sensor...");
}

void loop() {
  int irValue = digitalRead(IR_PIN);
  int potValue = analogRead(POT_PIN);
  int Brightness = map(potValue, 0, 4095, 0, 255);
  float voltage = (potValue / 4095.0) * 3.3;       // Calculates actual voltage (0V to 3.3V)
  if (irValue == LOW) {
    analogWrite(LED_PIN, Brightness);
    if (flag == 0) {
        flag = 1;
    Serial.println("IR sensor detected an object!");
      } 
    }else {
    analogWrite(LED_PIN, 0);
    if (flag == 1) {
        flag = 0;
        Serial.println("IR sensor did not detect any object.");
    }
  }
 
    Serial.print("IR Reading: ");
    Serial.print(irValue == LOW ? "OBJECT" : "CLEAR ");
    Serial.print(" | Pot Value: ");
    Serial.print(potValue);
    Serial.print(" | Voltage: ");
    Serial.print(voltage, 2);
    Serial.println("V");
    delay(100); // Delay for stability
  }
