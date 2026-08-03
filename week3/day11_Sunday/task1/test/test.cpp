#include <WiFi.h>
#include <PubSubClient.h>


const char* ssid = "CYBER_EXT";
const char* password = "cyberap2025";


const char* mqttServer = "broker.emqx.io";
const int mqttPort = 8884;


WiFiClient espClient;
PubSubClient client(espClient);



void setup() 
{
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) 
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  client.setServer(mqttServer, mqttPort);
  if (client.connect("ESP32_1"))
  {
    Serial.println("MQTT Connected");
    client.publish("esp32/test","Hello from ESP32");
  }
}

void loop() 
{
  client.loop();
}