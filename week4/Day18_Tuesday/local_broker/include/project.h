#ifndef PROJECT_H
#define PROJECT_H
#include "config.h"
#include "L298N.h"



void applyMotorHardware(L298N &motor, String &targetDirection, uint8_t &targetSpeedPer, bool &targetPower, PubSubClient &client);
void publishMotorStatus(L298N &motor, PubSubClient &client);
void mqttCallback(char *topic, byte *payload, unsigned int length);
void ensureConnections();
#endif