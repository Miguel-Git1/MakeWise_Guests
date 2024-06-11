#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFiClientSecure.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <esp_camera.h>
#include <SPIFFS.h>

const char *ssid = "WI-7-2.4";
const char *pass = "inov84ever";

const char *server = "https://spot.makewise.vision/api/process/fD50i3hPckJtPGCS7ihABeuk3bVjDYcM/9RZtqX1MdYqAw3uh";



#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27
#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22


void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_SVGA;   // The frame will be 800x600bl
  config.pixel_format = PIXFORMAT_JPEG; // for streaming
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 1;

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }

  if (!SPIFFS.begin(true))
  {
    Serial.println("\nSPIFFS initialisation failed!\n");
  }
  else
  {
    Serial.printf("\nDone with SPIFFS!\n");
  }
  for (int i = 0; i < 5; i++)
  {
    camera_fb_t *fb = NULL;
    fb = esp_camera_fb_get();

    if (!fb)
    {
      esp_camera_fb_return(fb);
      fb = NULL;
      Serial.printf("\n--- ERROR: Camera capture failed");
    }
    else
    {
      esp_camera_fb_return(fb);
      Serial.println("\n--- Good Image ---");
    }
  }
}




void setup() {
  //Initialize serial and wait for port to open:
  Serial.begin(115200);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  setupCamera();
  delay(100);


  Serial.print("Attempting to connect to SSID: ");
  Serial.println(ssid);
  WiFi.begin(ssid, pass);

  // attempt to connect to Wifi network:
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    // wait 1 second for re-trying
    delay(1000);
  }

  Serial.print("Connected to ");
  Serial.println(ssid);

  
}

void loop() {
WiFiClientSecure *client = new WiFiClientSecure;
  if(client) {
    client->setInsecure();
    HTTPClient https;



    //Initializing an HTTPS communication using the secure client
    Serial.print("[HTTPS] begin...\n");
    if (https.begin(*client, server)) {  // HTTPS
      Serial.print("[HTTPS] POST...\n");

      camera_fb_t *fb = esp_camera_fb_get();
      if (!fb)
      {
        Serial.printf("\nFailed to get frame.");
      }
      uint8_t *fbBuf = fb->buf;
      size_t fbLen = fb->len;

      https.addHeader("accept", "*/*");
      https.addHeader("Content-Type", "image/jpeg");
      https.addHeader("Content-Length", (String)fbLen);

      int httpCode = https.POST(fbBuf, fbLen);


      if (httpCode > 0) {

        Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          Serial.println(payload);

          if (payload == "Image accepted")
          {
            Serial.println("-- Image was a success");
          }
          
        }
        else
        {
          Serial.printf("\nNo payload code");
        }
      }
      else {
        Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    }
  }
  else {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  Serial.println();
  Serial.println("Waiting 2min before the next round...");
  delay(120000);
}
