#include <Arduino.h>
#include <WiFi.h>

const char *ssid = "Uai Fai";
const char *password = "liamba1234";

WiFiServer server(80);
String header;

// Led State
String outputState = "off";

// Led pin
const int output = 2;

void setup()
{
  Serial.begin(9600);
  pinMode(output, OUTPUT);
  digitalWrite(output, LOW);
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
  WiFiClient client = server.available();

  if (client)
  {
    Serial.println("New Client.");
    String currentLine = "";
    while (client.connected())
    {
      char c = client.read();
      Serial.write(c);
      header += c;
      if (c == '\n')
      {
        if (currentLine.length() == 0)
        {
          client.println("HTTP/1.1 200 OK");
          client.println("Content-type:text/html");
          client.println();

          if (header.indexOf("GET /26/on") >= 0)
          {
            Serial.println("GPIO 26 on");
            outputState = "on";
            digitalWrite(output, HIGH);
          }
          else if (header.indexOf("GET /26/off") >= 0)
          {
            Serial.println("GPIO 26 off");
            outputState = "off";
            digitalWrite(output, LOW);
          }

          client.println("<!DOCTYPE html><html>");
          client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
          client.println("<link rel=\"icon\" href=\"data:,\">");
          // CSS to style the on/off buttons
          // Feel free to change the background-color and font-size attributes to fit your preferences
          client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
          client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
          client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
          client.println(".button2 {background-color: #555555;}</style></head>");

          // Web Page Heading
          client.println("<body><h1>ESP32 Web Server</h1>");

          client.println("<p> GPIO - State " + outputState + "</p>");
          if (outputState == "off")
          {
            client.println("<p><a href=\"/26/on\"><button class=\"button\">ON</button></a></p>");
          }
          else
          {
            client.println("<p><a href=\"/26/off\"><button class=\"button button2\">OFF</button></a></p>");
          }
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
