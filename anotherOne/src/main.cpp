#include <TensorFlowLite_ESP32.h>
#include "detection_responder.h"
#include "image_provider.h"
#include "model_settings.h"
#include "person_detect_model_data.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "WiFi.h"
#include "SPIFFS.h" 

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <esp_log.h>
#include "esp_main.h"
#include <esp_camera.h>
#include "sd_read_write.h"

// ===================  
// Select camera model  
// ===================  
//#define CAMERA_MODEL_WROVER_KIT // Has PSRAM  
//#define CAMERA_MODEL_ESP_EYE // Has PSRAM  
#define CAMERA_MODEL_ESP32S3_EYE // Has PSRAM  
//#define CAMERA_MODEL_M5STACK_PSRAM // Has PSRAM  
//#define CAMERA_MODEL_M5STACK_V2_PSRAM // M5Camera version B Has PSRAM  
//#define CAMERA_MODEL_M5STACK_WIDE // Has PSRAM  
//#define CAMERA_MODEL_M5STACK_ESP32CAM // No PSRAM  
//#define CAMERA_MODEL_M5STACK_UNITCAM // No PSRAM  
//#define CAMERA_MODEL_AI_THINKER // Has PSRAM  
//#define CAMERA_MODEL_TTGO_T_JOURNAL // No PSRAM  
// ** Espressif Internal Boards **  
//#define CAMERA_MODEL_ESP32_CAM_BOARD  
//#define CAMERA_MODEL_ESP32S2_CAM_BOARD  
 
// Has to start here for some reason 
#include <camera_pins.h> 


void init_Wifi_SPIF(void);
void init_Camera(void);




const char index_html[] PROGMEM = R"rawliteral( 
<!DOCTYPE HTML><html> 
<head>   
  <meta name="viewport" content="width=device-width, initial-scale=1"> 
  <style> 
    body { text-align:center; } 
    .vert { margin-bottom: 10%; } 
    .hori{ margin-bottom: 0%; } 
  </style> 
</head> 
<body> 
  <div id="container"> 
    <h2>ESP32-CAM Last Photo</h2> 
    <p>It might take more than 5 seconds to capture a photo.</p> 
    <p> 
      <button onclick="rotatePhoto();">ROTATE</button> 
      <button onclick="capturePhoto()">CAPTURE PHOTO</button> 
      <button onclick="location.reload();">REFRESH PAGE</button> 
    </p> 
  </div> 
  <div><img src="saved-photo" id="photo" width="70%"></div> 
</body> 
<script> 
  var deg = 0; 
  function capturePhoto() { 
    var xhr = new XMLHttpRequest(); 
    xhr.open('GET', "/capture", true); 
    xhr.send(); 
  } 
  function rotatePhoto() { 
    var img = document.getElementById("photo"); 
    deg += 90; 
    if(isOdd(deg/90)){ document.getElementById("container").className = "vert"; } 
    else{ document.getElementById("container").className = "hori"; } 
    img.style.transform = "rotate(" + deg + "deg)"; 
  } 
  function isOdd(n) { return Math.abs(n % 2) == 1; } 
</script> 
</html>)rawliteral"; 
 



// Globals, used for compatibility with Arduino-style sketches.
namespace {
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;




#ifdef CONFIG_IDF_TARGET_ESP32S3
constexpr int scratchBufSize = 39 * 1024;
#else
constexpr int scratchBufSize = 0;
#endif
// An area of memory to use for input, output, and intermediate arrays.
constexpr int kTensorArenaSize = 81 * 1024 + scratchBufSize;
static uint8_t *tensor_arena;//[kTensorArenaSize]; // Maybe we should move this to external
}  // namespace

const char* ssid = "WI-7-2.4";
const char* password = "inov84ever";







// This function all it does is creating the model itself
void setup() {
  Serial.begin(115200);
  init_Camera();
  //init_Wifi_SPIF();



  // Create a microErrorReporter
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  //Get The Model (GTM)
  model = tflite::GetModel(model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    TF_LITE_REPORT_ERROR(error_reporter,
                         "Model provided is schema version %d not equal "
                         "to supported version %d.",
                         model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Allocate memory i guess
  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }

  // Define Layers
  static tflite::MicroMutableOpResolver<5> micro_op_resolver;
  micro_op_resolver.AddAveragePool2D();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddDepthwiseConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddSoftmax();

  // Create an MicroInterpreter with all the variables defined above
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory from the tensor_arena for the model's tensors.
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "AllocateTensors() failed");
    return;
  }

  // Get information about the memory area to use for the model's input.
  input = interpreter->input(0);

  // Initialize Camera
  TfLiteStatus init_status = InitCamera(error_reporter);
  if (init_status != kTfLiteOk) {
    TF_LITE_REPORT_ERROR(error_reporter, "InitCamera failed\n");
    return;
  }
}




// The name of this function is important for Arduino compatibility.
void loop() {
  // If everything is blown to pieces
  if (kTfLiteOk != GetImage(error_reporter, kNumCols, kNumRows, kNumChannels,
                            input->data.int8)) {
    TF_LITE_REPORT_ERROR(error_reporter, "Image capture failed.");
  }
  if (kTfLiteOk != interpreter->Invoke()) {
    TF_LITE_REPORT_ERROR(error_reporter, "Invoke failed.");
  }

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t person_score = output->data.uint8[kPersonIndex];
  int8_t no_person_score = output->data.uint8[kNotAPersonIndex];

  float person_score_f =
      (person_score - output->params.zero_point) * output->params.scale;
  float no_person_score_f =
      (no_person_score - output->params.zero_point) * output->params.scale;

  // Respond to detection
  RespondToDetection(error_reporter, person_score_f, no_person_score_f);
  vTaskDelay(1); // to avoid watchdog trigger
}

#if defined(COLLECT_CPU_STATS)
  long long total_time = 0;
  long long start_time = 0;
  extern long long softmax_total_time;
  extern long long dc_total_time;
  extern long long conv_total_time;
  extern long long fc_total_time;
  extern long long pooling_total_time;
  extern long long add_total_time;
  extern long long mul_total_time;
#endif

void run_inference(void *ptr) {
  /* Convert from uint8 picture data to int8 */
  for (int i = 0; i < kNumCols * kNumRows; i++) {
    input->data.int8[i] = ((uint8_t *) ptr)[i] ^ 0x80;
  }

#if defined(COLLECT_CPU_STATS)
  long long start_time = esp_timer_get_time();
#endif
  // Run the model on this input and make sure it succeeds.
  if (kTfLiteOk != interpreter->Invoke()) {
    error_reporter->Report("Invoke failed.");
  }

#if defined(COLLECT_CPU_STATS)
  long long total_time = (esp_timer_get_time() - start_time);
  printf("Total time = %lld\n", total_time / 1000);
  //printf("Softmax time = %lld\n", softmax_total_time / 1000);
  printf("FC time = %lld\n", fc_total_time / 1000);
  printf("DC time = %lld\n", dc_total_time / 1000);
  printf("conv time = %lld\n", conv_total_time / 1000);
  printf("Pooling time = %lld\n", pooling_total_time / 1000);
  printf("add time = %lld\n", add_total_time / 1000);
  printf("mul time = %lld\n", mul_total_time / 1000);

  /* Reset times */
  total_time = 0;
  //softmax_total_time = 0;
  dc_total_time = 0;
  conv_total_time = 0;
  fc_total_time = 0;
  pooling_total_time = 0;
  add_total_time = 0;
  mul_total_time = 0;
#endif

  TfLiteTensor* output = interpreter->output(0);

  // Process the inference results.
  int8_t person_score = output->data.uint8[kPersonIndex];
  int8_t no_person_score = output->data.uint8[kNotAPersonIndex];

  float person_score_f =
      (person_score - output->params.zero_point) * output->params.scale;
  float no_person_score_f =
      (no_person_score - output->params.zero_point) * output->params.scale;
  RespondToDetection(error_reporter, person_score_f, no_person_score_f);
}



void init_Wifi_SPIF() 
{ 
  WiFi.begin(ssid, password); 
  while (WiFi.status() != WL_CONNECTED) { 
    delay(1000); 
    Serial.println("Connecting to WiFi..."); 
  } 
  if (!SPIFFS.begin(true)) { 
    Serial.println("An Error has occurred while mounting SPIFFS"); 
    ESP.restart(); 
  } 
  else { 
    delay(500); 
    Serial.println("SPIFFS mounted successfully"); 
    sdmmcInit(); 
    createDir(SD_MMC, "/AIcontent");  
    listDir(SD_MMC, "/AIcontent", 0);  
  } 
 
   
  Serial.print("IP Address: http://"); 
  Serial.println(WiFi.localIP()); 
} 


void init_Camera() 
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
  config.frame_size = FRAMESIZE_UXGA;  
  config.pixel_format = PIXFORMAT_JPEG; // for streaming  
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;  
  config.fb_location = CAMERA_FB_IN_PSRAM;  
  config.jpeg_quality = 12;  
  config.fb_count = 1;  
 
  if (psramFound()) { 
    config.frame_size = FRAMESIZE_UXGA; 
    config.jpeg_quality = 10; 
    config.fb_count = 2; 
  } else { 
    config.frame_size = FRAMESIZE_SVGA;  
    config.fb_location = CAMERA_FB_IN_DRAM;  
  } 
  // Camera init 
  esp_err_t err = esp_camera_init(&config); 
  if (err != ESP_OK) { 
    Serial.printf("Camera init failed with error 0x%x", err); 
    ESP.restart(); 
  } 
  else 
  { 
    Serial.println("The camera was successfully staged!"); 
  } 
} 

