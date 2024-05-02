#include <Arduino.h>

// put function declarations here:
int led = 2;

void setup()
{
  // put your setup code here, to run once:
  pinMode(led, OUTPUT);
}

void loop()
{
  // put your main code here, to run repeatedly:
  digitalWrite(led, HIGH);
  delay(100);
  digitalWrite(led, LOW);
  delay(100);
}

// put function definitions here:
int myFunction(int x, int y)
{
  return x + y;
}