#include <Arduino.h>


int pinGPIO = 2;

void setup()
{
  pinMode(pinGPIO, OUTPUT);
}

void loop()
{
  delay(5000);
  digitalWrite(pinGPIO, HIGH);
  delay(5000);
  digitalWrite(pinGPIO, LOW);
}

// put function definitions here:
