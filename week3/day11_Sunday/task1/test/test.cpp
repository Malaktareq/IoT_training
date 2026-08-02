#include <WiFi.h>
#include <PubSubClient.h>

const char* ssid = "CYBER_EXT";
const char* password = "cyberap2025";

const char* mqttServer = "broker.emqx.io";
const int mqttPort = 8884;

WiFiClient espClient;
PubSubClient client(espClient);
void callback(char* topic, byte* payload, unsigned int length);
void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("WiFi connected");

  client.setServer(mqttServer, mqttPort);
  client.setCallback(callback);
  if (client.connect("ESP32_2"))
  {
    Serial.println("MQTT Connected");
    client.subscribe("esp32/test");
    client.publish("esp32/test", "Hello from ESP32");
  }
}

void callback(char* topic, byte* payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.println("]");


  Serial.print("Message: ");


  for(int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
  }


  Serial.println();
}

void loop()
{

Serial.println("Looping...");
  client.loop();
}