#ifndef ClassLed_h
#define Classled_h
#include <Arduino.h>
class Led
{
public:
    Led(int pin);
    void turnOn();
    void turnOff();
    void switchState();
    bool state();

private:
    int led;
    bool isOn;
};
#endif