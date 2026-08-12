#include "../include/L298N.h"
#include "../include/project.h"

L298N motor(IN1_PIN, IN2_PIN, EN_PIN);
Adafruit_BME280 bme;
WiFiClient espClient;
PubSubClient client(espClient);

bool targetPower = false;
String targetDirection = "forward";
uint8_t targetSpeedPer = 0;
unsigned long lastMsgTime = 0;
const unsigned long msgInterval = 2000; // 1 second interval for publishing sensor data

void setup()
{
    Serial.begin(115200);
    motor.begin();
    Wire.begin(SDA_PIN, SCL_PIN);
    if (!bme.begin(0x76))
    {
        if (!bme.begin(0x77))
            Serial.println("Could not find a valid BME280 sensor, check wiring!");
    }
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    client.setServer(MQTT_BROKER, MQTT_PORT);
    client.setCallback(mqttCallback);

    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.println("WiFi connected");
}

void loop()
{
    unsigned long currentMillis;
    float temperature, humidity, pressure;

    ensureConnections();
    client.loop();
    currentMillis = millis();
    if (currentMillis - lastMsgTime >= msgInterval)
    {
        lastMsgTime = currentMillis;
        if (WiFi.status() == WL_CONNECTED && client.connected())
        {
            temperature = bme.readTemperature();
            humidity = bme.readHumidity();
            pressure = bme.readPressure() / 100.0F;

            client.publish("esp32/sensor/temperature", String(temperature).c_str());
            client.publish("esp32/sensor/humidity", String(humidity).c_str());
            client.publish("esp32/sensor/pressure", String(pressure).c_str());
        }
    }
    else
    {
        applyMotorHardware(motor, targetDirection, targetSpeedPer, targetPower, client);
        delay(100);
    }
}



void mqttCallback(char *topic, byte *payload, unsigned int length)
{
    String message;
    String topicStr;
    int SpeedValue;

    for (unsigned int i = 0; i < length; i++)
        message += (char)payload[i];
    message.trim();
    topicStr = String(topic);

    if (topicStr == "esp32/motor/cmd/power")
        targetPower = (message == "1" || message == "on" || message == "ON");
    else if (topicStr == "esp32/motor/cmd/direction")
    {
        if (message == "forward" || message == "backward")
            targetDirection = message;
    }
    else if (topicStr == "esp32/motor/cmd/speed")
    {
        SpeedValue = message.toInt();
        targetSpeedPer = constrain(SpeedValue, 0, 100); // Constrain the speed percentage to be between 0 and 100 if less than 0 or greater than 100, set to 0 or 100 respectively
    }

    applyMotorHardware(motor, targetDirection, targetSpeedPer, targetPower, client);
    publishMotorStatus(motor, client);
}

void ensureConnections()
{
    unsigned long currentMillis;
    if (WiFi.status() == WL_CONNECTED && !client.connected())
    {
        applyMotorHardware(motor, targetDirection, targetSpeedPer, targetPower, client);
        client.connect("ESP32Client","esp32/status",1,true,"offline");
        if (client.connected())
        {
            client.subscribe("esp32/motor/cmd/#");
            client.publish("esp32/status", "online", true);
            publishMotorStatus(motor, client);
        }
    }
}
