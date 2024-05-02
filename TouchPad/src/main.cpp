#include <Arduino.h>
#include "button.h"
#include "led.h"

Led bruno(2);
Button mathew(4, 20, 50);
void setup()
{
  Serial.begin(9600);
  delay(1000); // give me time to bring up serial monitor
}

void loop()
{
  mathew.update();
  if (mathew.state())
  {
    bruno.switchState();
  }
  delay(500);
}
