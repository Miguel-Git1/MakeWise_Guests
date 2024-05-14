//
// A simple server implementation showing how to:
//  * serve static messages
//  * read GET and POST parameters
//  * handle missing pages / 404s
//

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#endif
#include <Adafruit_BME280.h>
#include <Wire.h>
#include <SPI.h>
#include <ESPAsyncWebServer.h>
using namespace std;

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10
#define SEALEVELPRESSURE_HPA (1018)

Adafruit_BME280 bme; // use I2C interface
AsyncWebServer server(80);

const char *ssid = "WI-7-2.4";
const char *password = "inov84ever";

const char *PARAM_MESSAGE = "message";

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}
String html();
double getTemperature();
double getHumidity();
double getPressure();
double getAltitude();
String getUpdate();
void setup()
{

  Serial.begin(115200);
  unsigned status;
  status = bme.begin(0x76);
  if (!status)
  {
    Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!");
    Serial.print("SensorID was: 0x");
    Serial.println(bme.sensorID(), 16);
    Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n");
    Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n");
    Serial.print("        ID of 0x60 represents a BME 280.\n");
    Serial.print("        ID of 0x61 represents a BME 680.\n");
    while (1)
      delay(10);
  }

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() != WL_CONNECTED)
  {
    Serial.printf("WiFi Failed!\n");
    return;
  }

  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/html", html()); });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(200, "text/plain", getUpdate()); });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
        String message;
        if (request->hasParam(PARAM_MESSAGE)) {
            message = request->getParam(PARAM_MESSAGE)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, GET: " + message); });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request)
            {
        String message;
        if (request->hasParam(PARAM_MESSAGE, true)) {
            message = request->getParam(PARAM_MESSAGE, true)->value();
        } else {
            message = "No message sent";
        }
        request->send(200, "text/plain", "Hello, POST: " + message); });

  server.onNotFound(notFound);

  server.begin();
}

void loop()
{
}

String html()
{
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<title>Weather Station</title>";
  // Script
  html += "<script>";
  html += "setInterval(function() {";
  html += "  fetch('/update')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      var values = data.split(',');";
  html += "      document.getElementById('temperature').textContent = values[0];";
  html += "      document.getElementById('pressure').textContent = values[1];";
  html += "      document.getElementById('humidity').textContent = values[2];";
  html += "      document.getElementById('altitude').textContent = values[3];";
  html += "    });";
  html += "}, 5000);"; // Update every 5 seconds
  html += "</script>";
  // End Script
  html += "<style>";
  html += "body {font-family: Arial, sans-serif; margin: 0; padding: 20px;}";
  html += "h1 {text-align: center;}";
  html += ".container {display: flex; justify-content: center; align-items: center; height: 80vh;}";
  html += ".weather-info {border: 2px solid #ccc; border-radius: 10px; padding: 20px;}";
  html += ".weather-info p {margin: 10px 0;}";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<h1>Weather Station</h1>";
  html += "<div class='container'>";
  html += "<div class='weather-info'>";
  html += "<p><strong>Temperature:</strong><span id='temperature'>";
  html += getTemperature();
  html += "</span>&deg;C</p>";
  html += "<p><strong>Pressure:</strong><span id='pressure'>";
  html += getPressure();
  html += "</span>hPa</p>";
  html += "<p><strong>Humidity:</strong><span id='humidity'>";
  html += getHumidity();
  html += "</span>%</p>";
  html += "<p><strong>Altitude(aprox.):</strong><span id='altitude'>";
  html += getAltitude();
  html += "</span>m</p>";
  html += "</div>";
  html += "</div>";
  html += "</script>";
  html += "</body>";
  html += "</html>";
  return html;
}

double getTemperature() { return bme.readTemperature(); }
double getHumidity() { return bme.readHumidity(); }
double getPressure() { return bme.readPressure() / 100.0F; }
double getAltitude() { return bme.readAltitude(SEALEVELPRESSURE_HPA); }

String getUpdate()
{
  String data = String(getTemperature()) + "," + String(getPressure()) + "," + String(getHumidity()) +
                "," + String(getAltitude());
  return data;
}
