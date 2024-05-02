// Classe.cpp

#include "button.h"

Button::Button(int numbtn, int c, int debounceMs)
{
    btn = numbtn;
    config = c;
    debounce = debounceMs;
}

void Button::update()
{
    if (touchRead(btn) <= 20)
    {
        if (!isPressed)
        {
            isPressed = true;
            wasPressed = false;
            startPressed = millis();
        }
    }
    else
    {
        if (isPressed)
        {
            isPressed = false;
            time = millis();
            if (time - startPressed >= debounce)
            {
                wasPressed = true;
            }
        }
    }
}

bool Button::state()
{
    if (wasPressed)
    {
        wasPressed = false;
        return true;
    }
    else
    {
        return false;
    }
}
