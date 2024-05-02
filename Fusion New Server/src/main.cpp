#include <Arduino.h>
#include <WiFi.h>
#include "button.h"
#include "led.h"

Led bruno(2);
Button mathew(4, 20, 50);
const char *ssid = "Uai Fai";
const char *password = "liamba1234";

WiFiServer server(80);
String header;

// Led State
String outputState = "off";

// Led pin
void writeHtml(WiFiClient client);
void setup()
{
  Serial.begin(115200);
  bruno.turnOff();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("");
  Serial.print("Wifi connected.");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  server.begin();
}

void loop()
{
  mathew.update();
  if (mathew.state())
  {
    bruno.switchState();
  }
  WiFiClient client = server.available();

  if (client)
  {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected())
    {
      mathew.update();
      if (mathew.state())
      {
        bruno.switchState();
      }

      char c = client.read();
      // Serial.write(c);
      header += c;
      if (c == '\n')
      {
        if (currentLine.length() == 0)
        {
          if (bruno.state() == true)
          {
            outputState = "ON";
          }
          else
          {
            outputState = "OFF";
          }
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          if (header.indexOf("GET /26/on") >= 0)
          {
            Serial.println("led on");
            outputState = "ON";
            bruno.turnOn();
          }
          else if (header.indexOf("GET /26/off") >= 0)
          {
            Serial.println("led off");
            outputState = "OFF";
            bruno.turnOff();
          }

          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<link rel=\"icon\" href=\"data:,\">");
          // CSS to style the on/off buttons
          // Feel free to change the background-color and font-size attributes to fit your preferences
          client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
          client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button2 {background-color: ##FF0400;}</style></head>");

          // Web Page Heading
          client.println("<body><h1>Simon's Playground</h1>");

          client.println("<p> GPIO - State " + outputState + "</p>");

          client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");

          client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");

          client.println("</body></html>");

          client.println();
          break;
        }
        else
        {
          currentLine = "";
        }
      }
      else if (c != '\r')
      {
        currentLine += c;
      }
    }
    header = "";
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}

void writeHtml(WiFiClient client)
{
  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");
  // CSS to style the on/off buttons
  // Feel free to change the background-color and font-size attributes to fit your preferences
  client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
  client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
  client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
  client.println(".button2 {background-color: ##FF0400;}</style></head>");

  // Web Page Heading
  client.println("<body><h1>Simon's Playground</h1>");

  client.println("<p> Led State " + outputState + "</p>");

  client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");

  client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");

  client.println("</body></html>");

  client.println();
}