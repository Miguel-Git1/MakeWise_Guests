#include <Arduino.h>
#include <FS.h>
#include <SPIFFS.h>
#include <esp_camera.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
#include <Wire.h>
#include "jpeg_decoder.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>


// Tensorflow
#include "tensorflow/lite/micro/kernels/micro_ops.h"
#include "tensorflow/lite/micro/tflite_bridge/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/micro/micro_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include <model_data.h> // Main Model



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

#define IMG_WIDTH 100 // Model input width

#define IMG_HEIGHT 75 // Model input height

int8_t imageBuffer[IMG_WIDTH * IMG_HEIGHT * 3];
AsyncWebServer server(80);

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

String inferCarImage(const int8_t *image)
{
  memcpy(input->data.int8, image, sizeof(imageBuffer));
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

int8_t uint8Toint8(uint8_t uint8color)
{
  return (int8_t)(((int)uint8color) - 128); // is there a better way?
}

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

void saveImg()
{
  Serial.println("Taking a photo...");
  camera_fb_t *fb = NULL;
  fb = esp_camera_fb_get();

  if (!fb)
  {
    esp_camera_fb_return(fb);
    fb = NULL;
    Serial.printf("Camera capture failed");
    return;
  }
  else
  {
    Serial.println("photo taken! Preprocessing image, this may take a while...");
  }


  File file = SPIFFS.open("/img.jpg", FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
    return;
  }
  else
  {
    file.write(fb->buf, fb->len); // payload (image), payload length
    Serial.printf("Saved file to path: /img.jpg With the size of: %d\n", fb->len);
  }
  file.close();
  unsigned char *decoded, *p, *o;
  int x, y, v;
  int decoded_outsize = IMG_WIDTH * IMG_HEIGHT * 3;

  decoded = (uint8_t *)malloc(decoded_outsize);
  for (x = 0; x < decoded_outsize; x += 2)
  {
    decoded[x] = 0;
    decoded[x + 1] = 0xff;
  }

  /* JPEG decode */
  esp_jpeg_image_cfg_t jpeg_cfg = {
      .indata = (uint8_t *)fb->buf,
      .indata_size = fb->len,
      .outbuf = decoded,
      .outbuf_size = decoded_outsize,
      .out_format = JPEG_IMAGE_FORMAT_RGB888,
      .out_scale = JPEG_IMAGE_SCALE_1_8,
      .flags = {
          .swap_color_bytes = 0,
      }};

  esp_jpeg_image_output_t outimg;
  esp_err_t err = my_esp_jpeg_decode(&jpeg_cfg, &outimg);

  unsigned long t = millis();
  for (int i = 0; i < 10; i++)
  {
    esp_err_t err = my_esp_jpeg_decode(&jpeg_cfg, &outimg);
  }
  Serial.printf("\n--- ESP JPEG DECODE TIMER: %d", (millis() - t) / 10);
  esp_camera_fb_return(fb); 


  file = SPIFFS.open("/img.raw", FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file in writing mode");
    return;
  }
  else
  {
    file.write(decoded, decoded_outsize); // payload (image), payload length
    Serial.println("Saved file to path: /img.raw");
  }
  file.close();

  

  for (int i = 0; i < decoded_outsize; i++)
  {
    imageBuffer[i] = uint8Toint8(decoded[i]);
  }



  Serial.printf("\n-- Consersion is done --\n");
  free(decoded);
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

  server.on("/raw", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.raw", "image/raw");; });
  server.on("/jpg", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/img.jpg", "image/jpg");; });
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


void setup()
{
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); // disable brownout detector
  Serial.begin(115200);
  setupCamera();
  Serial.println("DONE SETUP");
  delay(2000);
  saveImg();
  delay(2000);
  initWifiAndServer();
  delay(2000);
  initTFInterpreter();
  Serial.println("---- Ready! ---- ");
}


void loop()
{
  if (Serial.read() == 'w')
  {
    delay(3000);
    saveImg();
    String result = inferCarImage(imageBuffer);
    Serial.print("Testing License. Result:");
    Serial.println(result);
    delay(3000);
  };

  delay(10);
}
