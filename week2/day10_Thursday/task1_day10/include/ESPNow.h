#ifndef ESPNOW_H
#define ESPNOW_H
#include <esp_now.h>
#include "Message.h"
#include <WiFi.h>
class ESPNow {
public:
    ESPNow();
    bool begin();
    bool addPeer(const uint8_t *peerAddress);
    bool send(const uint8_t *peerAddress, const uint8_t *data, size_t size);
    void onReceive(void (*callback)(const uint8_t *mac, const uint8_t *incomingData, int len));
    void onSend(void (*callback)(const uint8_t *mac_addr, esp_now_send_status_t status));
};

#endif