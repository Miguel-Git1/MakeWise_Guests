#include <Arduino.h>
#include <model_data.h>
#include <cstdio>
#include <car.h>
#include <iostream>
#include <stdio.h>
#include "esp_heap_caps.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

//const int scratchBufSize = 39 * 1024;
const int kTensorArenaSize = 200 * 1024; //+ scratchBufSize;
//static tflite::MicroErrorReporter micro_error_reporter;
static uint8_t* tensor_arena = nullptr;
tflite::ErrorReporter* error_reporter = nullptr;
const tflite::Model* model = nullptr;
tflite::MicroInterpreter* interpreter = nullptr;
TfLiteTensor* input = nullptr;
TfLiteTensor* output = nullptr;


void printText();
void convertToInt8(const uint8_t *input_data, int8_t *output_data);

void setup() {
  Serial.begin(112500);
  Serial.printf("\n KtensorArenaSize: %d Bytes \n", kTensorArenaSize);
  Serial.printf("\n KtensorArenaSize: %d KB \n", (kTensorArenaSize / 1024));
  delay(1500);
  // Other Variables with same adress
  static tflite::MicroErrorReporter micro_error_reporter;
  error_reporter = &micro_error_reporter;
  




  // Get the model
  model = tflite::GetModel(quant_model_tflite);

  // Create the micro_op_resolver

  
  static tflite::MicroMutableOpResolver<5> micro_op_resolver;
  micro_op_resolver.AddRelu();
  micro_op_resolver.AddConv2D();
  micro_op_resolver.AddReshape();
  micro_op_resolver.AddFullyConnected();
  micro_op_resolver.AddSoftmax();
  Serial.printf("\nLayers Are Initialized! \n");
  

  

  Serial.printf("\nINIT FREE ---------------------------------------- \n");
  printText();

  delay(1000);

  // Create Tensor Arena Size
  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) ps_calloc(1, kTensorArenaSize);
    if (tensor_arena != NULL)
    {
      Serial.printf("\nThe tensor arena is located in: %p", &tensor_arena);
    }
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }



  delay(1000);


  static tflite::MicroInterpreter static_interpreter(
    model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);
  interpreter = &static_interpreter;




  TfLiteStatus allocate_status = interpreter->AllocateTensors();

  input = interpreter->input(0);

  // Invoke  
  if (interpreter->Invoke() != kTfLiteOk) {
    error_reporter->Report("Invoke failed.");
    return;
  }
  output = interpreter->output(0);
  float value;


  input->data.f[0] = 0.;
  interpreter->Invoke();
  value = output->data.f[0];
  Serial.printf("\n (10) Value: %f ", value);
  

  

  // FINAL RAM
  Serial.printf("\n\nFINAL FREE ---------------------------------------- \n");
  printText();





// Salamalecos
}

void loop() {

}


void printText()
{
  Serial.printf("Total heap: %d", ESP.getHeapSize());
  Serial.printf("\nFree heap: %d", ESP.getFreeHeap());
  Serial.printf("\nTotal PSRAM: %d", ESP.getPsramSize());
  Serial.printf("\nFree PSRAM: %d", ESP.getFreePsram());
  Serial.printf("\nPSRAM After Tensor Arena: %d", ESP.getPsramSize() - kTensorArenaSize);
  Serial.printf("\nTotal PSRAM - FREE PSRAM: %d KB", (ESP.getPsramSize() - ESP.getFreePsram()) / 1024);
  Serial.printf("\n----------------------------------------\n\n");
}





