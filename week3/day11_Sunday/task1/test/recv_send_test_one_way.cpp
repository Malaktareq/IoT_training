#include <esp_now.h>
// #include <WiFi.h>

// uint8_t partnerMAC[6] = {0x78, 0x42, 0x1C, 0x6C, 0x13, 0x68};

// typedef struct struct_message
// {
//     char a[32];
//     int b;
//     float c;
//     bool d;
// } struct_message;

// struct_message myData;
// esp_now_peer_info_t peerInfo;

// void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status)
// {
//     Serial.print("\r\nLast Packet Send Status:\t");
//     Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
// }

// void setup()
// {
//     Serial.begin(115200);
//     WiFi.mode(WIFI_STA);
//     Serial.println("ESPNow/Basic/Master Example");
//     Serial.print("STA MAC: ");
//     Serial.println(WiFi.macAddress());
//     if (esp_now_init() != ESP_OK)
//     {
//         Serial.println("Error initializing ESP-NOW");
//         return;
//     }
//     esp_now_register_send_cb(OnDataSent);
//     memcpy(peerInfo.peer_addr, partnerMAC, 6);
//     peerInfo.channel = 0;
//     peerInfo.encrypt = false;
//     if (esp_now_add_peer(&peerInfo) != ESP_OK)
//     {
//         Serial.println("Failed to add peer");
//         return;
//     }
// }

// void loop()
// {
//     strcpy(myData.a, "Hello, World!");
//     myData.b = 42;
//     myData.c = 3.14;
//     myData.d = true;

//     if (esp_now_send(partnerMAC, (uint8_t *)&myData, sizeof(myData)) != ESP_OK)
//     {
//         Serial.println("Error sending data");
//     }
//     delay(1000);
// }

#include <WiFi.h>

// Structure example to receive data
// Must match the sender structure
typedef struct struct_message {
    char a[32];
    int b;
    float c;
    bool d;
} struct_message;

// Create a struct_message called myData
struct_message myData;

// callback function that will be executed when data is received
void OnDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
  memcpy(&myData, incomingData, sizeof(myData));
  Serial.print("Bytes received: ");
  Serial.println(len);
  Serial.print("Char: ");
  Serial.println(myData.a);
  Serial.print("Int: ");
  Serial.println(myData.b);
  Serial.print("Float: ");
  Serial.println(myData.c);
  Serial.print("Bool: ");
  Serial.println(myData.d);
  Serial.println();
}
 
void setup() {
  // Initialize Serial Monitor
  Serial.begin(115200);
  
  // Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  
  // Once ESPNow is successfully Init, we will register for recv CB to
  // get recv packer info
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnDataRecv));
}
 
void loop() {

}