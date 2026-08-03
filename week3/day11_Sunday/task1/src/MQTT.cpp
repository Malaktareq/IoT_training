#include "../include/MQTT.h"

MQTT::MQTT() : mqttClient(wifiClient) {}

bool MQTT::begin()
{
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
    while(WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println("WiFi connected");
    mqttClient.setServer(MQTT_SERVER,MQTT_PORT);
    
    if(mqttClient.connect("ESP32_MQTT"))
    {
        Serial.println("MQTT connected");
        mqttClient.subscribe(MQTT_SUBSCRIBE_TOPIC);
        return true;
    }
    Serial.println("MQTT connection failed");
    return false;
}

void MQTT::loop()
{
    mqttClient.loop();
}

bool MQTT::publish(const char* topic, const char* payload)
{
    if(mqttClient.connected())
        return mqttClient.publish(topic, payload);
    Serial.println("MQTT not connected, cannot publish");
    return false;
}

