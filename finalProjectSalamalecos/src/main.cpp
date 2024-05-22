#include <Arduino.h>
#include "main_functions.h"
#include <Wire.h>
#include <SPI.h>
#include <SPIFFS.h>
#include <WiFiClient.h>
#include "soc/soc.h"          // Disable brownout problems
#include "soc/rtc_cntl_reg.h" // Disable brownout problems
#include "esp_heap_caps.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include "FS.h"
#include <LittleFS.h>
#include <SD.h>



// WEB
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
//  CAM
#include "esp_camera.h"
// Model
#include "model_data.h"
#include "carData.h"
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
// #include "tensorflow/lite/version.h"

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

#define IMG_WIDHT 96  // Model input width
#define IMG_HEIGHT 96 // Model input height

bool switchFormat = true;
int8_t *imageBuffer = new int8_t[img_data_length]{0};
sensor_t *ccSensor = nullptr;
camera_fb_t *fb96 = nullptr;
AsyncWebServer server(80);

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
</head>
<body>
  <h2>ESP Image Web Server</h2>
  <div><img src="img"></div>
</body>  
</html>)rawliteral";

namespace
{
  tflite::ErrorReporter *error_reporter = nullptr;
  const tflite::Model *model = nullptr;
  tflite::MicroInterpreter *interpreter = nullptr;
  TfLiteTensor *input = nullptr;
  // An area of memory to use for input, output, and intermediate arrays.
  const int kTensorArenaSize = 70 * 1024;
  static uint8_t *tensor_arena;
  using AllOpsResolver = tflite::MicroMutableOpResolver<128>;
  TfLiteStatus RegisterOps(AllOpsResolver &resolver)
  {

    resolver.AddConv2D();
    resolver.AddFullyConnected();
    resolver.AddRelu();
    resolver.AddReshape();
    resolver.AddSoftmax();

    return TfLiteStatus::kTfLiteOk;
  }
}


void initSerial()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Serial.setRxBufferSize(1024);
  Serial.begin(115200);
  Serial.setTimeout(10000);
}

void initWifiAndServer()
{
  WiFi.begin("WI-7-2.4", "inov84ever");
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  // Route for root / web page
  Serial.print("IP Address: http://");
  Serial.println(WiFi.localIP());

  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
  //   { request->send_P(200, "text/html", index_html); });

  server.on("/img", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.jpg", "image/jpg"); });
  server.begin();
  Serial.println("Server begin");
}

void initTFInterpreter()
{

  if (tensor_arena == NULL)
  {
    tensor_arena = (uint8_t *)heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (tensor_arena == NULL)
  {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  // Get the model
  model = tflite::GetModel(quant_model_tflite);
  if (model->version() != TFLITE_SCHEMA_VERSION)
  {
    error_reporter->Report(
        "Model provided is schema version %d not equal "
        "to supported version %d.",
        model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  AllOpsResolver op_resolver;
  RegisterOps(op_resolver);

  static tflite::MicroInterpreter static_interpreter(
      model, op_resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk)
  {
    error_reporter->Report("AllocateTensors() failed");
    return;
  }

  input = interpreter->input(0);
  error_reporter->Report("Input Shape");
  for (int i = 0; i < input->dims->size; i++)
  {
    error_reporter->Report("%d", input->dims->data[i]);
  }

  error_reporter->Report(TfLiteTypeGetName(input->type));
  error_reporter->Report("Output Shape");

  TfLiteTensor *output = interpreter->output(0);
  for (int i = 0; i < output->dims->size; i++)
  {
    error_reporter->Report("%d", output->dims->data[i]);
  }
  error_reporter->Report(TfLiteTypeGetName(output->type));
  error_reporter->Report("Arena Used:%d bytes of memory", interpreter->arena_used_bytes());
}


static camera_config_t config;
void setupCamera()
{
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
  
  config.pixel_format = PIXFORMAT_RGB565; // for streaming
  config.frame_size = FRAMESIZE_96X96;    // The frame will be 96x96
  


  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  
  Serial.printf("\n\n--- It Got here.\n\n");

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("--- ERROR: Camera init failed with error 0x%x", err);
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_brightness(s, 1);
  s->set_saturation(s, 0);

  if (!SPIFFS.begin(true))
  {
    Serial.println("\n--- ERROR: SPIFFS initialisation failed!\n");
  }
  else
  {
    Serial.printf("\n--- Done with SPIFFS!\n");
  }
}


void setupCameraJPG()
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


  config.pixel_format = PIXFORMAT_JPEG; 
  config.frame_size = FRAMESIZE_240X240;       


  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.jpeg_quality = 12;
  config.fb_count = 2;

  Serial.printf("\n\n--- It Got here.\n\n");

  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK)
  {
    Serial.printf("--- ERROR: Camera init failed with error 0x%x", err);
  }

  sensor_t *s = esp_camera_sensor_get();
  s->set_vflip(s, 1);
  s->set_brightness(s, 1);
  s->set_saturation(s, 0);

  if (!SPIFFS.begin(true))
  {
    Serial.println("\n--- ERROR: SPIFFS initialisation failed!\n");
  }
  else
  {
    Serial.printf("\n--- Done with SPIFFS!\n");
  }
}


void setup()
{
  initSerial();
  delay(2000);
  setupCamera();
  delay(2000);
  //initWifiAndServer();
  delay(2000);
  initTFInterpreter();
  Serial.println("Ready");
  Serial.println("--- Press the letter P to start --- ");
}

float calculate_probabilities(int8_t logit)
{
  int value;
  value = static_cast<int>(logit);
  value += 128;
  value = value / 2.55;
  return value;
}

String PercentualDecode(TfLiteTensor *layer)
{
  int probabilities[2];
  error_reporter->Report("Model score: %d %d", layer->data.int8[0], layer->data.int8[1]);

  probabilities[0] = calculate_probabilities(layer->data.int8[0]);
  probabilities[1] = calculate_probabilities(layer->data.int8[1]);
  error_reporter->Report("Probabilities: %d %d", probabilities[0], probabilities[1]);
  if (probabilities[0] > probabilities[1])
  {
    return "No License";
  }
  else
  {
    return "License";
  }
}

int8_t uint8Toint8(uint8_t uint8color)
{
  return (int8_t)(((int)uint8color) - 128); // is there a better way?
}

typedef struct
{
  uint8_t r;
  uint8_t g;
  uint8_t b;
} rgb;

rgb rgb565_rgb888(uint16_t RGB16)
{
  rgb out;
  out.r = (RGB16 & 0b1111100000000000) >> 8;
  out.g = (RGB16 & 0b11111100000) >> 3;
  out.b = (RGB16 & 0b11111) << 3;
  return out;
}

void preprocessImage(camera_fb_t *fb, uint8_t *rgb888Image, int8_t *outputBuffer, int outputWidth, int outputHeight)
{
  int originalWidth = fb->width;
  int originalHeight = fb->height;
  // uint8_t *pixelData = fb->buf; // Assuming RGB888 format

  Serial.printf("\nBytes before conversion: ");
  for (int i = 0; i < 30; i++)
  {
    Serial.printf(" %d", rgb888Image[i]);
  }

  // Resize without converting to grayscale
  for (int y = 0; y < outputHeight; y++)
  {
    for (int x = 0; x < outputWidth; x++)
    {
      int originalX = (int)((float)x * originalWidth / outputWidth);
      int originalY = (int)((float)y * originalHeight / outputHeight);
      int pixelIndex = (originalY * originalWidth + originalX) * 3; // RGB888

      // Copy RGB values directly
      outputBuffer[(y * outputWidth + x) * 3] = rgb888Image[pixelIndex];
      outputBuffer[(y * outputWidth + x) * 3 + 1] = rgb888Image[pixelIndex + 1];
      outputBuffer[(y * outputWidth + x) * 3 + 2] = rgb888Image[pixelIndex + 2];
    }
  }
  Serial.printf("\n\nBytes after conversion: ");
  for (int i = 0; i < 30; i++)
  {
    Serial.printf(" %d", outputBuffer[i]);
  }
}

void Saveimg(camera_fb_t *fb)
{
  File file = SPIFFS.open("/img.jpg", FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
    return;
  }
  else
  {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.println("\n--- Saved file to path: /img.jpg");
    Serial.printf("\n--- Image captured: %d bytes\n", fb->len);
  }
  file.close();
}


bool takePicProcessPic()
{
  fb96 = esp_camera_fb_get();
  if (!fb96)
  {
    esp_camera_fb_return(fb96);
    fb96 = NULL;
    Serial.printf("\n--- ERROR: Camera capture failed \n");
    return false;
  }
  else
  {
    Serial.println("\n-- The requested photo has been taken! \n--- Preprocessing image, this may take a moment...");
  }
  Serial.printf("Free heap: %d\n", ESP.getFreeHeap());
  Serial.printf("Free PSRAM: %d\n", ESP.getFreePsram());

  if (fb96)
  {
    size_t imageSize = fb96->width * fb96->height * 3; // RGB888
    uint8_t *rgb888_image = (uint8_t *)malloc(imageSize);
    if (rgb888_image == NULL)
    {
      Serial.println("--- ERROR: Memory allocation failed");
      esp_camera_fb_return(fb96);
      return false;
    }
    Serial.println("--- Converting to RGB888...");
    uint16_t *rgb565_pixel = (uint16_t *)fb96->buf;


    Serial.printf("\n--- Bytes before RGB888: ");
    for (int i = 0; i < 30; i++)
    {
      Serial.printf(" %d", rgb565_pixel[i]);
    }
    for (int i = 0; i < fb96->width * fb96->height; i++)
    {
      rgb pixel = rgb565_rgb888(rgb565_pixel[i]);
      rgb888_image[i * 3] = pixel.r;
      rgb888_image[i * 3 + 1] = pixel.g;
      rgb888_image[i * 3 + 2] = pixel.b;
    }

    preprocessImage(fb96, rgb888_image, imageBuffer, IMG_WIDHT, IMG_HEIGHT);
    delay(10);
    free(rgb888_image);


    Serial.println("\n--- Complete!");
    esp_camera_fb_return(fb96);
    fb96 = NULL;
    delay(5000);
  }
  else
  {
    Serial.println("--- ERROR: Can't process without image!");
  }
  return true;
}

String inferCarImage(const int8_t *image)
{
  memcpy(input->data.int8, image, img_data_length);
  for (int i = 0; i < img_data_length; i++)
  {
    // input->data.int8[i] = uint8Toint8(image[i]); // For Preloaded Images.
    input->data.int8[i] = image[i];
  }
  int start = millis();
  error_reporter->Report("Invoking.");

  if (kTfLiteOk != interpreter->Invoke()) // Any error i have in invoke tend to just crash the whole system so i dont usually see this message
  {
    error_reporter->Report("Invoke failed.");
  }
  else
  {
    error_reporter->Report("Invoke passed.");
    error_reporter->Report(" Took :");
    Serial.print(millis() - start);
    error_reporter->Report(" milliseconds");
  }

  TfLiteTensor *output = interpreter->output(0);
  String result = PercentualDecode(output);

  return result;
}

void testPreloadedImg()
{
  Serial.print("Testing License. Result:");

  // String result = inferCarImage(no_license);
  Serial.print("Testing License. Result:");
  // Serial.println(result);
}

/*
int hexadecimalToDecimal(string hexVal)
{
    int len = hexVal.size();

    // Initializing base value to 1, i.e 16^0
    int base = 1;

    int dec_val = 0;

    // Extracting characters as digits from last
    // character
    for (int i = len - 1; i >= 0; i--) {
        // if character lies in '0'-'9', converting
        // it to integral 0-9 by subtracting 48 from
        // ASCII value
        if (hexVal[i] >= '0' && hexVal[i] <= '9') {
            dec_val += (int(hexVal[i]) - 48) * base;

            // incrementing base by power
            base = base * 16;
        }

        // if character lies in 'A'-'F' , converting
        // it to integral 10 - 15 by subtracting 55
        // from ASCII value
        else if (hexVal[i] >= 'A' && hexVal[i] <= 'F') {
            dec_val += (int(hexVal[i]) - 55) * base;

            // incrementing base by power
            base = base * 16;
        }
    }
    return dec_val;
}
*/

void loop()
{
  delay(500);
  if (Serial.read() == 'p')
  {
    //testPreloadedImg();

    bool done = takePicProcessPic();
    esp_err_t deinit = esp_camera_deinit();
    if (done)
    {
      String result = inferCarImage(imageBuffer);
      Serial.printf("--- Final Result: %s\n", result);
      delay(1000);
      if(deinit == ESP_OK)
      {
        Serial.printf("\n\n--- Camera was deinitialized");
        delay(5000);
        setupCameraJPG();
      }
      else
      {
        Serial.printf("\n\n--- ERROR: Camera didn't deinitialized");
      }
    }
    else
    {
      Serial.printf("\n\n--- ERROR: False was returned\n\n");
    }
    Serial.println("\n\n--- Press the letter P to start ---\n");
  }
}