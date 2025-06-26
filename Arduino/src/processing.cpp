#include <Arduino.h>
#include <arduinoFFT.h>
#include <math.h>
#include <stdint.h>

#include "audio.h"
#include "model_data.h"
#include "processing.h"

#define FRAME_LEN 255
#define FRAME_STEP 128
#define FFT_SIZE 256
#define NUM_FRAMES 124
#define FREQ_BINS 129

constexpr int kTensorArenaSize = 120000;
uint8_t tensor_arena[kTensorArenaSize];

arduinoFFT FFT;
tflite::MicroInterpreter* interpreter;
tflite::MicroMutableOpResolver<5> resolver;

void initModel() {
  Serial.println("Lodaing model version 4.");
  const tflite::Model* model = tflite::GetModel(model_tflit);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    Serial.println("Model schema version mismatch!");
    while (1)
      ;
  }

  resolver.AddFullyConnected();
  resolver.AddConv2D();
  resolver.AddResizeBilinear();
  resolver.AddMaxPool2D();
  resolver.AddReshape();

  // Build the interpreter
  static tflite::MicroInterpreter static_interpreter(
      model, resolver, tensor_arena, kTensorArenaSize);
  interpreter = &static_interpreter;

  // Allocate memory for tensors
  TfLiteStatus allocate_status = interpreter->AllocateTensors();
  if (allocate_status != kTfLiteOk) {
    Serial.println("Failed to allocate tensors!");
    while (1)
      ;
  }

  Serial.println("TFLite Micro interpreter initialized!");

  randomSeed(analogRead(A0));
}

void compute_quantized_spectrogram(TfLiteTensor* input) {
  double vReal[FFT_SIZE];
  double vImag[FFT_SIZE];

  float scale = input->params.scale;
  int zero_point = input->params.zero_point;

  int8_t* input_data = input->data.int8;

  Serial.print("Scale: ");
  Serial.println(scale, 6);
  Serial.print("Zero point: ");
  Serial.println(zero_point);

  for (int frame = 0; frame < NUM_FRAMES; frame++) {
    int start = frame * FRAME_STEP;

    for (size_t i = 0; i < FFT_SIZE; i++) {
      if (i < FRAME_LEN && (start + i) < RECORD_BUFFER_SIZE) {
        vReal[i] = recordBuffer[i];
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

      if (frame == 10) {
        Serial.print(mag);
        if (bin % 50 == 49) {
          Serial.println();
        } else {
          Serial.print(", ");
        }
      }
      int quantized = round((mag / scale) + zero_point);
      quantized = constrain(quantized, -128, 127);
      int index = frame * FREQ_BINS + bin;
      input_data[index] = quantized;
    }
  }
  Serial.println();
  Serial.println("\nInput data:");
  for (int i = 0; i < 600; i++) {
    Serial.print(input_data[i]);
    if (i % 50 == 49) {
      Serial.println();
    } else {
      Serial.print(", ");
    }
  }
  Serial.println();
}

bool runInference() {
  TfLiteTensor* input = interpreter->input(0);
  Serial.print("Input shape");
  Serial.println(input->dims->data[0]);
  Serial.println(input->dims->data[1]);
  Serial.println(input->dims->data[2]);
  Serial.println(input->dims->data[3]);
  compute_quantized_spectrogram(input);

  if (interpreter->Invoke() != kTfLiteOk) {
    Serial.println("Inference failed!");
    return false;
  }

  TfLiteTensor* output = interpreter->output(0);
  Serial.print("Output scale: ");
  Serial.println(output->params.scale, 6);
  Serial.print("Output zero point: ");
  Serial.println(output->params.zero_point);

  // Example for classification output
  int8_t* output_data = output->data.int8;
  float scale = output->params.scale;
  int zero_point = output->params.zero_point;

  int num_classes = output->dims->data[1];

  float scores[num_classes];
  float max_score = -INFINITY;

  // Dequantize scores
  for (int i = 0; i < num_classes; i++) {
    scores[i] = (output_data[i] - zero_point) * scale;
    if (scores[i] > max_score) {
      max_score = scores[i];
    }
  }

  float sum_exp = 0.0;
  for (int i = 0; i < num_classes; i++) {
    scores[i] = exp(scores[i] - max_score);  // for numerical stability
    sum_exp += scores[i];
  }

  int best_index = -1;
  float best_prob = -1.0;

  Serial.println("Class probabilities:");
  for (int i = 0; i < num_classes; i++) {
    float probability = scores[i] / sum_exp;
    Serial.print("Class ");
    Serial.print(i);
    Serial.print(": ");
    Serial.println(probability, 6);

    if (probability > best_prob) {
      best_prob = probability;
      best_index = i;
    }
  }

  return true;
}

void processData(byte const* data, size_t size) {}