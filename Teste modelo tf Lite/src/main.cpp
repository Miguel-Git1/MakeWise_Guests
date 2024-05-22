#include <Arduino.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/micro/micro_error_reporter.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/schema/schema_generated.h>
#include "carData.h"
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
// Include your model data
extern const unsigned char g_license_model_data[];
extern const unsigned int g_license_model_data_len;

// Tensor Arena Size
const int kTensorArenaSize = 128 * 1024; // 128KB for tensor arena

// Globals
tflite::ErrorReporter *error_reporter;
tflite::MicroInterpreter *interpreter;
const tflite::Model *model;
uint8_t *tensor_arena;

void convertToInt8(const uint8_t *input_data, int8_t *output_data, size_t length);
void logMemoryUsage()
{
  Serial.printf("Heap Size: %u\n", ESP.getHeapSize());
  Serial.printf("Free Heap: %u\n", ESP.getFreeHeap());
  Serial.printf("PSRAM Size: %u\n", ESP.getPsramSize());
  Serial.printf("Free PSRAM: %u\n", ESP.getFreePsram());
}

void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup...");

  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  // Initialize the error reporter
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;

  // Get the model
  model = tflite::GetModel(g_license_model_data);
  if (model->version() != TFLITE_SCHEMA_VERSION)
  {
    error_reporter->Report("Model schema version %d is not equal to supported version %d.",
                           model->version(), TFLITE_SCHEMA_VERSION);
    return;
  }

  // Create the micro op resolver
  static tflite::MicroMutableOpResolver<5> micro_op_resolver;
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddSoftmax();
  Serial.println("Layers are initialized!");

  // Check for PSRAM
  if (psramFound())
  {
    Serial.println("PSRAM found and initialized");
  }
  else
  {
    Serial.println("PSRAM not found");
    return;
  }

  Serial.println("\n------------------AFTER LAYERS----------------------");
  logMemoryUsage();
  Serial.println("------------------------------------------------------\n");

  delay(1000);

  // Allocate tensor arena in PSRAM
  tensor_arena = (uint8_t *)ps_calloc(1, kTensorArenaSize);
  if (tensor_arena == NULL)
  {
    error_reporter->Report("Couldn't allocate memory of %d bytes", kTensorArenaSize);
    return;
  }

  Serial.printf("\nThe tensor arena is located at: %p\n", tensor_arena);

  Serial.println("\n-------------AFTER ARENA-----------------");
  logMemoryUsage();
  Serial.println("----------------------------------------\n");

  // Create the interpreter
  static tflite::MicroInterpreter static_interpreter(
      model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;

  // Allocate memory for the model's tensors
  if (interpreter->AllocateTensors() != kTfLiteOk)
  {
    error_reporter->Report("AllocateTensors() failed");
    return;
  }

  Serial.println("\n-----------AFTER INTERPRETER---------------");
  logMemoryUsage();
  Serial.println("----------------------------------------\n");

  TfLiteStatus allocate_status = static_interpreter.AllocateTensors();

  TfLiteTensor *input = interpreter->input(0);

  // Ensure input dimensions match your model
  Serial.printf("Input dimensions: %d x %d x %d\n", input->dims->data[1], input->dims->data[2], input->dims->data[3]);

  //  Copy image data to input tensor
  for (int i = 0; i < input->bytes; i++)
  {
    input->data.int8[i] = ((uint8_t *)car_data)[i] - 128;
  }
  for (int i = 0; i < 10; i++)
  {
    Serial.println(input->data.int8[i]);
  }
  // Run the model on the input data
  if (interpreter->Invoke() != kTfLiteOk)
  {
    error_reporter->Report("Invoke failed.");
    return;
  }
  TfLiteTensor *output = interpreter->output(0);
  // Output the results
  Serial.println("Inference completed successfully!");
  for (int i = 0; i < output->dims->data[1]; i++)
  {
    Serial.printf("Output %d: %f\n", i, output->data.f[i]);
    Serial.println("Setup completed successfully!");
  }
}
void loop()
{
}

void convertToInt8(const uint8_t *input_data, int8_t *output_data, size_t length)
{
  Serial.print("Entrei na função");
  for (size_t i = 0; i < length; ++i)
  {
    output_data[i] = static_cast<int8_t>(input_data[i]) - 128;
  }
  Serial.print("Terminei o ciclo");
}