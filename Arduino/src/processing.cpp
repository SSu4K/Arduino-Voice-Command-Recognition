#include <arduinoFFT.h>
#include <Arduino.h>
#include <math.h>
#include <stdint.h>

#include "audio.h"
#include "processing.h"
#include "model_data.h"

#define FRAME_LEN 255
#define FRAME_STEP 128
#define FFT_SIZE 256
#define NUM_FRAMES 124
#define FREQ_BINS 129

constexpr int kTensorArenaSize = 2 * 1024;
uint8_t tensor_arena[kTensorArenaSize];

arduinoFFT FFT;
tflite::MicroInterpreter* interpreter;
tflite::MicroMutableOpResolver<5> resolver;

void initModel() {
  const tflite::Model* model = tflite::GetModel(model_tflit);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema version mismatch!");
    while (1)
      ;
  }

  resolver.AddFullyConnected();
  resolver.AddReshape();
  resolver.AddSoftmax();
  resolver.AddConv2D();
  resolver.AddMul();

  // Build the interpreter
  static tflite::MicroInterpreter static_interpreter(model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory for tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("Failed to allocate tensors!");
    while (1)
      ;
  }

  Serial.println("TFLite Micro interpreter initialized!");
}

void compute_quantized_spectrogram(TfLiteTensor* input) {
  double vReal[FFT_SIZE];
  double vImag[FFT_SIZE];

  float scale = input->params.scale;
  int zero_point = input->params.zero_point;

  int8_t* input_data = input->data.int8;

  for (int frame = 0; frame < NUM_FRAMES; frame++) {
    int start = frame * FRAME_STEP;

    for (size_t i = 0; i < FFT_SIZE; i++) {
      if (i < FRAME_LEN && (start + i) < RECORD_BUFFER_SIZE) {
        vReal[i] = (double)recordBuffer[start + i];
      } else {
        vReal[i] = 0.0;
      }
      vImag[i] = 0.0;
    }

    FFT.Windowing(vReal, FFT_SIZE, FFT_WIN_TYP_HAMMING, FFT_FORWARD);
    FFT.Compute(vReal, vImag, FFT_SIZE, FFT_FORWARD);

    for (int bin = 0; bin < FREQ_BINS; bin++) {
      double mag = sqrt(vReal[bin] * vReal[bin] + vImag[bin] * vImag[bin]);
      // Optional: mag = log1p(mag);

      int quantized = round((mag / scale) + zero_point);
      quantized = constrain(quantized, -128, 127);

      // Flattened index for [1][124][129][1]
      input_data[frame * FREQ_BINS + bin] = (int8_t)quantized;
    }
  }
}

bool runInference(tflite::MicroInterpreter* interpreter) {
  TfLiteTensor* input = interpreter->input(0);
  compute_quantized_spectrogram(input);

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Inference failed!");
    return false;
  }

  TfLiteTensor* output = interpreter->output(0);
  
  // Example for classification output
  int8_t* output_data = output->data.int8;
  float scale = output->params.scale;
  int zero_point = output->params.zero_point;

  int best_index = -1;
  float best_score = -INFINITY;

  for (int i = 0; i < output->dims->data[1]; i++) {
    float score = (output_data[i] - zero_point) * scale;
    if (score > best_score) {
      best_score = score;
      best_index = i;
    }
  }

  Serial.print("Prediction: class ");
  Serial.print(best_index);
  Serial.print(" score: ");
  Serial.println(best_score);

  return true;
}


void processData(byte const * data, size_t size){

}