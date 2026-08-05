#include <Arduino.h>
#include "../include/MQTT.h"
#include "../include/L298N.h"


MQTT mqt;
L298N motor(MOTOR_DIR_PIN2, MOTOR_SPEED_PIN1, MOTOR_ENA);
void mqttCallback(char *topic, byte *payload, unsigned int length);

unsigned long volatile lastDetectionTime;
const int detectionTimeout = 2000;

void pinsSetup();
void setup() 
{
  Serial.begin(115200);
  pinsSetup();
  if (!mqt.begin()) 
  {
    Serial.println("Error initializing MQTT");
    return;
  }
  mqt.getClient().setCallback(mqttCallback);
  lastDetectionTime = millis();
  motor.forward(255);
}

void loop()
{
  if (millis() - lastDetectionTime > detectionTimeout) 
  {
    digitalWrite(RUNNING_LED_PIN, LOW);
    digitalWrite(STOP_LED_PIN, HIGH);
    motor.stop();
  }
  else if (digitalRead(IR_PIN) == HIGH)
  {
    Serial.println("NO Object detected");
    mqt.publish(MQTT_PUBLISH_TOPIC, "1");
  }
  else
  {
    Serial.println("Object detected");
    mqt.publish(MQTT_PUBLISH_TOPIC, "0");
  }
  mqt.getClient().loop();
}

void pinsSetup() 
{
  pinMode(IR_PIN, INPUT);
  pinMode(RUNNING_LED_PIN, OUTPUT);
  pinMode(STOP_LED_PIN, OUTPUT);
  motor.begin();
}

void mqttCallback(char *topic, byte *payload, unsigned int length) 
{
  if (String(topic) != MQTT_SUBSCRIBE_TOPIC) {
    return;
  }

  String message;
  lastDetectionTime = millis();
  for (unsigned int i = 0; i < length; i++) 
  {
    message += (char)payload[i];
  }
  Serial.print("Received message: ");
  Serial.println(message);

  if (message == "1") 
  {
    digitalWrite(RUNNING_LED_PIN, HIGH);
    digitalWrite(STOP_LED_PIN, LOW);
    motor.forward(255);
  } 
  else if (message == "0") 
  {
    digitalWrite(RUNNING_LED_PIN, LOW);
    digitalWrite(STOP_LED_PIN, HIGH);
    motor.stop();
  }
}