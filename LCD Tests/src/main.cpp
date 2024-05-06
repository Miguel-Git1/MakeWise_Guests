/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com
*********/

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_Sensor.h>

#define DHTPIN 4
#define DHTTYPE DHT11

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
DHT dht(DHTPIN, DHTTYPE);
// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);
void setup()
{
  Serial.begin(115200);
  dht.begin();
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C))
  { // Address 0x3D for 128x64
    Serial.println(F("SSD1306 allocation failed"));
    for (;;)
      ;
  }
  delay(2000);
  display.clearDisplay();

  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 10);
}

void loop()
{
  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)

  // Check if any reads failed and exit early (to try again).
  if (isnan(h) || isnan(t) || isnan(f))
  {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }

  Serial.print(F("Humidity: "));
  Serial.print(h);
  Serial.print(F("%  Temperature: "));
  Serial.print(t);
  Serial.print(F("°C "));

  /*sensor.read();
  switch (sensor.getState())
  {
  case DHT_OK:
    Serial.print("Temperature = ");
    Serial.print(sensor.getTemperatureC());
    Serial.print("Humidity = ");
    Serial.print(sensor.getHumidity());
    Serial.println(" %");
    display.print("Temperature = ");
    display.print(sensor.getTemperatureC());
    display.print("Humidity = ");
    display.print(sensor.getHumidity());
    display.println(" %");
    break;
  case DHT_ERROR_CHECKSUM:
    Serial.println("Checksum error");
    display.println("Checksum error");
    break;
  case DHT_ERROR_TIMEOUT:
    Serial.println("Time out error");
    display.println("Time out error");
    break;
  case DHT_ERROR_NO_REPLY:
    Serial.println("Sensor not connected");
    display.println("Sensor not connected");
    break;
  }
  display.display();
  delay(2000);
  */
}

void scroll()
{
  display.startscrollright(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrollleft(0x00, 0x0F);
  delay(2000);
  display.stopscroll();
  delay(1000);
  display.startscrolldiagright(0x00, 0x07);
  delay(2000);
  display.startscrolldiagleft(0x00, 0x07);
  delay(2000);
  display.stopscroll();
  delay(1000);
}