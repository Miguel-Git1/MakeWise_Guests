#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_BME280.h>
#include <string>
#include <cmath>

// Define the number of devices we have in the chain and the hardware interface
// NOTE: These pin numbers will probably not work with your hardware and may
// need to be adapted
#define HARDWARE_TYPE MD_MAX72XX::FC16_HW

#define MAX_DEVICES 4

#define CLK_PIN   13
#define DATA_PIN  27
#define CS_PIN    25

#define BME_SCK 16
#define BME_SDA 4
#define SEALEVELPRESSURE_HPA (1013.25)

void printValues();


// Ada Fruit
Adafruit_BME280 bme; // use I2C interface
Adafruit_Sensor *bme_temp = bme.getTemperatureSensor();
Adafruit_Sensor *bme_pressure = bme.getPressureSensor();
Adafruit_Sensor *bme_humidity = bme.getHumiditySensor();


bool trade = false;

// Hardware SPI connection
//MD_Parola P = MD_Parola(HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

void setup(void)
{
  Serial.begin(115200);
  
  unsigned status;

  status = bme.begin(0x76);


    if (!status) {
        Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
        Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16);
        Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
        Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
        Serial.print("        ID of 0x60 represents a BME 280.\n");
        Serial.print("        ID of 0x61 represents a BME 680.\n");
        while (1) delay(10);
    }
    
    Serial.println("-- Default Test --");

    Serial.println();
  P.begin();
  P.displayClear();
}

int level = 0;
void loop(void)
{
  std::string humidity = "H: " + std::to_string(ceil(bme.readHumidity()* 100) / 100);
  const char * h = humidity.c_str();

  std::string temperature = "T: " + std::to_string(ceil(bme.readTemperature()* 100) / 100);
  const char * t = temperature.c_str();

  std::string Pressure = "P: " + std::to_string(ceil(bme.readPressure()*100 / 100));
  const char * p = Pressure.c_str();


  
  if (P.displayAnimate())
  {
    if(level == 0)
    {
      P.displayText(t, PA_CENTER, 200, 1000, PA_OPENING_CURSOR, PA_CLOSING_CURSOR);
    }
    else if (level == 1)
    {
      P.displayText(h, PA_CENTER, 200, 1000, PA_OPENING_CURSOR, PA_CLOSING_CURSOR);
    }
    else if (level == 2)
    {
      P.displayText(p, PA_CENTER, 200, 1000, PA_OPENING_CURSOR, PA_CLOSING_CURSOR);
    }
    if (level == 3)
    {
      level = 0;
    }
    else
    {
      level++;
    }
    
  }



}







void printValues() {
    Serial.print("Temperature = ");  Serial.print(bme.readTemperature());
    Serial.println(" °C");

    Serial.print("Pressure = ");

    Serial.print(bme.readPressure() / 100.0F);
    Serial.println(" hPa");

    Serial.print("Approx. Altitude = ");
    Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
    Serial.println(" m");

    Serial.print("Humidity = ");
    Serial.print(bme.readHumidity());
    Serial.println(" %");

    Serial.println();
}