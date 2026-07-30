#include <Arduino.h>
#include "../include/ESPNow.h"
#include "../include/L298N.h"


ESPNow espnow;
L298N motor(MOTOR_DIR_PIN2, MOTOR_SPEED_PIN1, MOTOR_ENA);

const uint8_t partnerMAC[6] = {0x00, 0x70, 0x07, 0xA3, 0xDA, 0x14}; // Replace with the MAC address of the partner device
bool Detected = false;
unsigned long lastDetectionTime;
const int detectionTimeout = 2000;

void OnDataSent(const uint8_t *mac, esp_now_send_status_t status);
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len);
void pinsSetup();
void setup() 
{
  Serial.begin(115200);
  pinsSetup();
  if (!espnow.begin()) 
  {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  if (!espnow.addPeer(partnerMAC)) 
  {
    Serial.println("Error adding peer");
    return;
  }
  espnow.onSend(OnDataSent);
  espnow.onReceive(OnDataRecv);
  lastDetectionTime = millis();
}

void loop()
{
  message_t msg;
  Serial.println(digitalRead(IR_PIN));
  if (millis() - lastDetectionTime > detectionTimeout)
  {
    motor.stop();
    digitalWrite(RUNNING_LED_PIN, LOW);
    digitalWrite(STOP_LED_PIN, HIGH);
  }
  if (digitalRead(IR_PIN) == HIGH) 
  {
      Serial.println("Object detected.");
      msg.senderID = 1; // Set the sender ID
      msg.runMotor = true; // Set the runMotor flag to true
      msg.timestamp = millis(); // Set the timestamp
      if (!espnow.send(partnerMAC, (uint8_t *)&msg, sizeof(msg))) 
        Serial.println("Error sending message");
      digitalWrite(RUNNING_LED_PIN, HIGH); // Turn on the running LED
      digitalWrite(STOP_LED_PIN, LOW); // Turn off the stop LED
  } 
  else 
  {
      Serial.println("Object no longer detected.");
      msg.senderID = 1; // Set the sender ID
      msg.runMotor = false; // Set the runMotor flag to false
      msg.timestamp = millis(); // Set the timestamp
      if (!espnow.send(partnerMAC, (uint8_t *)&msg, sizeof(msg))) 
        Serial.println("Error sending message");
      digitalWrite(RUNNING_LED_PIN, LOW); // Turn off the running LED
      digitalWrite(STOP_LED_PIN, HIGH); // Turn on the stop LED
    }
}

void OnDataSent(const uint8_t *mac,esp_now_send_status_t status)
{
    if(status == ESP_NOW_SEND_SUCCESS)
    {
        Serial.println("Delivery Success");
    }
    else
    {
        Serial.println("Delivery Failed");
    }
}

void pinsSetup() 
{
  pinMode(IR_PIN, INPUT);
  pinMode(RUNNING_LED_PIN, OUTPUT);
  pinMode(STOP_LED_PIN, OUTPUT);
  motor.begin();
}

void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len)
{
  message_t msg;
  
  if(len != sizeof(message_t))
  {
    Serial.println("Wrong packet size");
    return;
  }
  
  lastDetectionTime = millis();
  memcpy(&msg, incomingData, sizeof(msg));
  Serial.print("Received message: ");
  Serial.println(msg.senderID);
  Serial.print("Run Motor: ");
  Serial.println(msg.runMotor);
  if (msg.runMotor) 
  {
    motor.forward(255); // Move forward at full speed
    digitalWrite(STOP_LED_PIN, LOW); // Turn off the stop LED
    digitalWrite(RUNNING_LED_PIN, HIGH); // Turn on the running LED
  } 
  else 
  {
    motor.stop(); // Stop the motor
    digitalWrite(RUNNING_LED_PIN, LOW); // Turn off the running LED
    digitalWrite(STOP_LED_PIN, HIGH); // Turn on the stop LED
  }
}