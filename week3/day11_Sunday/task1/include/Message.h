#ifndef MESSAGE_H
#define MESSAGE_H

#include <cstdint>

typedef struct message {
  uint8_t senderID;
  bool runMotor;
  uint32_t timestamp;
}message_t;



#endif