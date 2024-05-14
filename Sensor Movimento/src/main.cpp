#include <Arduino.h>
#include "led.h"
#include "movesensor.h"

MoveSensor sensor(35);
Led led(15);

/**
 * Will block until a high signal is detected, which length is equal or above the given one
 */

void setup()
{
  led.turnOff();
  Serial.begin(115200);
}

void loop()
{
  delay(500);
  if (sensor.state() == true)
  {

    led.turnOn();
    Serial.println("Detected!!");
  }
  else
  {
    led.turnOff();
    Serial.println("Scanning...");
  }
}