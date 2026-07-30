#include "../include/ESPNow.h"

ESPNow::ESPNow() 
{
}

bool ESPNow::begin() 
{
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK)
    {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }
    return true;
}

bool ESPNow::addPeer(const uint8_t *peerAddress) 
{
    esp_now_peer_info_t peerInfo ={};
    memcpy(peerInfo.peer_addr, peerAddress, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    if (esp_now_add_peer(&peerInfo) != ESP_OK)
    {
        Serial.println("Failed to add peer");
        return false;
    }
    return true;
}

bool ESPNow::send(const uint8_t *peerAddress, const uint8_t *data, size_t size) 
{
    if (esp_now_send(peerAddress, data, size) != ESP_OK)
    {
        Serial.println("Error sending data");
        return false;
    }
    return true;
}
 
void ESPNow::onReceive(void (*callback)(const uint8_t *mac, const uint8_t *incomingData, int len)) 
{
    esp_now_register_recv_cb(callback);
}

void ESPNow::onSend(void (*callback)(const uint8_t *mac_addr, esp_now_send_status_t status)) 
{
    esp_now_register_send_cb(callback);
}
