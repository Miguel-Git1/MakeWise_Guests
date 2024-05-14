#include "movesensor.h"

MoveSensor::MoveSensor(int pin)
{
    pinMode(pin, INPUT);
    sensor = pin;
}
bool MoveSensor::state()
{
    if (digitalRead(sensor) == 1)
    {
        return true;
    }
    else
    {
        return false;
    }
}