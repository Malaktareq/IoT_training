#include "../include/L298N.h"
#include "../include/project.h"
void applyMotorHardware(L298N &motor, String &targetDirection, uint8_t &targetSpeedPer, bool &targetPower, PubSubClient &client)
{
    uint8_t speed;
    if (WiFi.status() != WL_CONNECTED || !client.connected() || !targetPower)
    {
        motor.stop();
        return;
    }
    speed = map(targetSpeedPer, 0, 100, 0, 255);
    if (targetDirection == "forward")
        motor.forward(speed);
    else if (targetDirection == "backward")
        motor.reverse(speed);
    else
        motor.stop();
}

void publishMotorStatus(L298N &motor, PubSubClient &client)
{
    uint8_t speed;
    uint8_t direction;
    String dirstr;
    String stateStr;

    if (WiFi.status() != WL_CONNECTED || !client.connected())
        return;
    speed = motor.getSpeed();
    direction = motor.getDirection();

    dirstr = "stopped";
    if (direction == 1)
        dirstr = "forward";
    else if (direction == 2)
        dirstr = "backward";
    
    speed = map(speed, 0, 255, 0, 100);
    stateStr = (speed > 0 && direction != 0) ? "running" : "stopped";

    client.publish("esp32/motor/speed", String(speed).c_str());
    client.publish("esp32/motor/direction", dirstr.c_str());
    client.publish("esp32/motor/power", stateStr.c_str());
}