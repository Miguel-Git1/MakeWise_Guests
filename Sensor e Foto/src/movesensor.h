#ifndef CLassMoveSensor
#define CLassMoveSensor_h
#include <Arduino.h>
class MoveSensor
{
public:
    MoveSensor(int pin);
    bool state();

private:
    int sensor;
};
#endif