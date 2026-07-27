#include <Arduino.h>
#include "../include/CurrentSensor.h"
#include "../include/L298N.h"
#include "../include/enum_class.h"


void init_pins();
void Leds_Control(SystemState state);
void IRAM_ATTR emergency_stop_isr();
volatile unsigned long lastInterrupt = 0;
volatile unsigned long faultStartTime = 0;
const unsigned long OVERCURRENT_RECOVERY_DELAY_MS = 1000;

L298N motor(MOTOR_SPEED_PIN1, MOTOR_DIR_PIN2, MOTOR_ENA);
SystemState systemState;

volatile bool emergency_stop_triggered = false;
void setup()
{
  Serial.begin(115200);
  systemState = SystemState::RUNNING;
  init_pins();
  CurrentSensor_Init(SENSOR_PIN, CURRENT_SENSOR_SCALE);
  attachInterrupt(digitalPinToInterrupt(E_STOP_PIN), emergency_stop_isr, FALLING);
}

void loop()
{
  Leds_Control(systemState);
  if (emergency_stop_triggered)
  {
    systemState = SystemState::EMERGENCY_STOP;
    motor.stop();
    return;
  }
  if (CurrentSensor_IsOvercurrent(CURRENT_THRESHOLD, systemState))
  {
    faultStartTime = millis();
    systemState = SystemState::FAULT_OVERCURRENT;
    motor.stop();
    return;
  }
  if (systemState == SystemState::FAULT_OVERCURRENT)
  {
    if (millis() - faultStartTime < OVERCURRENT_RECOVERY_DELAY_MS)
    {
      motor.stop();
      return;
    }
  }
  systemState = SystemState::RUNNING;
  motor.forward(255);
}

void init_pins()
{
    motor.begin();
    pinMode(SENSOR_PIN, INPUT);
    pinMode(E_STOP_PIN, INPUT_PULLUP);
    pinMode(RUNNING_LED_PIN, OUTPUT);
    pinMode(STOP_LED_PIN, OUTPUT);
    pinMode(LOW_CURRENT_LED_PIN, OUTPUT);
    pinMode(OVERCURRENT_LED_PIN, OUTPUT);
}

void Leds_Control(SystemState state)
{
  switch (state)
  {
    case SystemState::STOP:
        digitalWrite(RUNNING_LED_PIN, LOW);
        digitalWrite(STOP_LED_PIN, HIGH);
        digitalWrite(LOW_CURRENT_LED_PIN, LOW);
        digitalWrite(OVERCURRENT_LED_PIN, LOW);
        break;
    case SystemState::RUNNING:
        digitalWrite(RUNNING_LED_PIN, HIGH);
        digitalWrite(STOP_LED_PIN, LOW);
        digitalWrite(LOW_CURRENT_LED_PIN, HIGH);
        digitalWrite(OVERCURRENT_LED_PIN, LOW);
        break;
    case SystemState::FAULT_OVERCURRENT:
        digitalWrite(RUNNING_LED_PIN, LOW);
        digitalWrite(STOP_LED_PIN, HIGH);
        digitalWrite(LOW_CURRENT_LED_PIN, LOW);
        digitalWrite(OVERCURRENT_LED_PIN, HIGH);
        break;
    case SystemState::EMERGENCY_STOP:
        digitalWrite(RUNNING_LED_PIN, LOW);
        digitalWrite(STOP_LED_PIN, HIGH);
        digitalWrite(LOW_CURRENT_LED_PIN, LOW);
        digitalWrite(OVERCURRENT_LED_PIN, LOW);
        break;
  }
}

void IRAM_ATTR emergency_stop_isr()
{
  unsigned long now = millis();

  if (now - lastInterrupt < 200)
      return;
  lastInterrupt = now;
  emergency_stop_triggered = !emergency_stop_triggered;
}
