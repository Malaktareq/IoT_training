#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>

struct message {
  uint8_t senderID;
  bool runMotor;
  uint32_t timestamp;
};

#endif