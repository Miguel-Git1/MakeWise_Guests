#include <Arduino.h>
#include <model_data.h>
#include <cstdio>
#include <stdio.h>
#include "esp_heap_caps.h"
#include <TensorFlowLite_ESP32.h>
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/micro/micro_mutable_op_resolver.h"
#include "tensorflow/lite/schema/schema_generated.h"

// Variables

//const int scratchBufSize = 39 * 1024;
const int kTensorArenaSize = 81 * 1024; //+ scratchBufSize;
tflite::ErrorReporter* error_reporter;
//static tflite::MicroErrorReporter micro_error_reporter;
static uint8_t* tensor_arena = nullptr;
const tflite::Model* model = nullptr;


void setup() {
  Serial.begin(112500);








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
  


  Serial.printf("\n\nAFTER LAYERS ---------------------------------------- \n");
  Serial.printf("Total heap: %d", ESP.getHeapSize());
  Serial.printf("\nFree heap: %d", ESP.getFreeHeap());
  Serial.printf("\nTotal PSRAM: %d", ESP.getPsramSize());
  Serial.printf("\nFree PSRAM: %d", ESP.getFreePsram());
  Serial.printf("\n----------------------------------------\n\n");






  heap_caps_free(tensor_arena);
  tensor_arena = NULL;

  // Create Tensor Arena Size
  if (tensor_arena == NULL) {
    tensor_arena = (uint8_t *) heap_caps_malloc(kTensorArenaSize, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (tensor_arena != NULL)
    {
      Serial.printf("\nThe tensor arena is located in: %p", &tensor_arena);
    }
  }
  if (tensor_arena == NULL) {
    printf("Couldn't allocate memory of %d bytes\n", kTensorArenaSize);
    return;
  }


  Serial.printf("\n\nAFTER ARENA ---------------------------------------- \n");
  Serial.printf("Total heap: %d", ESP.getHeapSize() + " bytes");
  Serial.printf("\nFree heap: %d", ESP.getFreeHeap() + " bytes");
  Serial.printf("\nTotal PSRAM: %d", ESP.getPsramSize() + " bytes");
  Serial.printf("\nFree PSRAM: %d", ESP.getFreePsram() + " bytes");
  Serial.printf("\n----------------------------------------\n\n");


  

  // Create the Interpreter 

  
  //static tflite::MicroInterpreter static_interpreter(
    //model, micro_op_resolver, tensor_arena, kTensorArenaSize, error_reporter);


/*
  TfLiteStatus allocate_status = static_interpreter.AllocateTensors();

  static_interpreter.input(0);

  static_interpreter.Invoke();


  static_interpreter.output(0);
  
*/
}

void loop() {

}


