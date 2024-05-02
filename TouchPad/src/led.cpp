
#include "led.h"

Led::Led(int pin)
{
    pinMode(pin, OUTPUT);
    led = pin;
}

void Led::turnOn()
{
    digitalWrite(led, HIGH);
    isOn = true;
}
void Led::turnOff()
{
    digitalWrite(led, LOW);
    isOn = false;
}

void Led::switchState()
{
    if (isOn == true)
    {
        digitalWrite(led, LOW);
        isOn = false;
    }
    else
    {
        digitalWrite(led, HIGH);
        isOn = true;
    }
}
bool Led::state()
{
    if (isOn == true)
    {
        return true;
    }
    else
    {
        return false;
    }
}
