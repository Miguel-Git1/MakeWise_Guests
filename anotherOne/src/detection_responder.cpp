#include "detection_responder.h"
#include "esp_main.h"

void RespondToDetection(tflite::ErrorReporter* error_reporter,
                        float person_score, float no_person_score) {
  int person_score_int = (person_score) * 100 + 0.5;

  TF_LITE_REPORT_ERROR(error_reporter, "person score:%d%%, no person score %d%%",
                       person_score_int, 100 - person_score_int);

}
