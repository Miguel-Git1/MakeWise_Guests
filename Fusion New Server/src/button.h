#ifndef ClassBtn_h
#define Classbtn_h
#include <Arduino.h>
class Button
{
public:
    Button(int numbtn, int c, int debounceMs);
    void update();
    bool state();

private:
    int btn;
    int config;
    bool isPressed;
    bool wasPressed;
    unsigned startPressed;
    int debounce;
    unsigned time;
};
#endif