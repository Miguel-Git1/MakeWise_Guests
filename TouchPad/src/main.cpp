#include <Arduino.h>

// Add the mouse pin
int mouse = 14;
int GPIO_led = 2;

void setup()
{
  Serial.begin(9600);
  delay(1000); // give me time to bring up serial monitor
  Serial.println("ESP32 Touch Test");
  pinMode(mouse, INPUT);
  pinMode(GPIO_led, OUTPUT);
}

void loop()
{
  if (touchRead(mouse) < 62) 
  {
    digitalWrite(GPIO_led, HIGH);
  }
  else 
  {
    digitalWrite(GPIO_led, LOW);
  }

}