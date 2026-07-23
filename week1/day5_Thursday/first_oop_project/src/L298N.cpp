#include "../include/L298N.h"

L298N::L298N(uint8_t &pin1, uint8_t &pin2, uint8_t &ena)
{
    this->_pin1 = pin1;
    this->_pin2 = pin2;
    _currentspeed = 0;
    _currentdirection = 0;
    _ena = ena;
}

void L298N::begin()
{
    pinMode(_pin1, OUTPUT);
    pinMode(_pin2, OUTPUT);
    pinMode(_ena, OUTPUT);
    stop();
}

void L298N::forward(uint8_t &speed)
{
    _currentspeed = speed;
    _currentdirection = 1;
    analogWrite(_ena, _currentspeed);
    digitalWrite(_pin1, HIGH);
    digitalWrite(_pin2, LOW);
}

void L298N::reverse(uint8_t &speed)
{
    _currentspeed = speed;
    _currentdirection = 2;
    analogWrite(_ena, _currentspeed);
    digitalWrite(_pin1, LOW);
    digitalWrite(_pin2, HIGH);
}

void L298N::stop()
{
    _currentspeed = 0;
    _currentdirection = 0;
    analogWrite(_ena, _currentspeed);
    digitalWrite(_pin1, LOW);
    digitalWrite(_pin2, LOW);
}

void L298N::Brake()
{
    _currentspeed = 0;
    _currentdirection = 0;
    analogWrite(_ena, _currentspeed);
    digitalWrite(_pin1, HIGH);
    digitalWrite(_pin2, HIGH);
}

