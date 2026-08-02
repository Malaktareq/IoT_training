#ifndef MQTT_H
#define MQTT_H
#include "Message.h"
#include <WiFi.h>
#include <PubSubClient.h>
#include "config.h"
#include "Arduino.h"
class MQTT
{
    private:
        WiFiClient wifiClient;
        PubSubClient mqttClient;
public:
    MQTT();
    bool begin();
    void loop();
    bool publish(const char* topic, const char* payload);
    PubSubClient& getClient() { return mqttClient; }
};

#endif