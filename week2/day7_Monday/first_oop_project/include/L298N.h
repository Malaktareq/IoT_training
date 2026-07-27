#ifndef L298N_H
#define L298N_H

#include "config.h"
class L298N {
private:
    uint8_t _pin1;
    uint8_t _pin2;
    uint8_t _currentspeed;
    uint8_t _currentdirection;
    uint8_t _ena;
public:
    L298N(const uint8_t &pin1, const uint8_t &pin2, const uint8_t &ena);
    void begin();
    void forward(uint8_t speed);
    void reverse(uint8_t &speed);
    void stop();
    void Brake();
};

#endif