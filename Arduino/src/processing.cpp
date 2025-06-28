#include <Arduino.h>
#include <arduinoFFT.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include "debug.h"
#include "frontend.h"
#include "frontend_util.h"

#include "audio.h"
#include "model_data.h"
#include "processing.h"

#define FRAME_LEN 255
#define FRAME_STEP 128
#define FFT_SIZE 256
#define NUM_FRAMES 124
#define FREQ_BINS 129

arduinoFFT FFT;

// Model globals
constexpr int kTensorArenaSize = 120000;
uint8_t tensor_arena[kTensorArenaSize];
tflite::MicroInterpreter* interpreter;
tflite::MicroMutableOpResolver<5> resolver;

// Preprocessor globals
struct FrontendConfig frontend_config;
struct FrontendState frontend_state;

void initModel() {
  DBG_PRINTLN("Lodaing model version 4.");
  const tflite::Model* model = tflite::GetModel(model_tflit);
  if (model->version() != TFLITE_SCHEMA_VERSION) {
    DBG_PRINTLN("Model schema version mismatch!");
    while (1)
      ;
  }
  DBG_PRINTLN("Loaded model!");

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
    DBG_PRINTLN("Failed to allocate tensors!");
    while (1)
      ;
  }

  DBG_PRINTLN("TFLite Micro interpreter initialized!");

  randomSeed(analogRead(A0));
}

void initPreprocessor() {
  FrontendFillConfigWithDefaults(&frontend_config);
  if (!FrontendPopulateState(&frontend_config, &frontend_state, SAMPLE_RATE)) {
    DBG_PRINTLN("Failed to populate frontend state\n");
  }
}

void computeQuantizedSpectrogram(TfLiteTensor* input) {
  if (input == NULL) {
    return;
  }

  double vReal[FFT_SIZE];
  double vImag[FFT_SIZE];

  float scale = input->params.scale;
  int zero_point = input->params.zero_point;

  int8_t* input_data = input->data.int8;

  DBG_PRINT("Scale: ");
  DBG_PRINTLN(scale, 6);
  DBG_PRINT("Zero point: ");
  DBG_PRINTLN(zero_point);

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
        DBG_PRINT(mag);
        if (bin % 50 == 49) {
          DBG_PRINTLN();
        } else {
          DBG_PRINT(", ");
        }
      }
      int quantized = round((mag / scale) + zero_point);
      quantized = constrain(quantized, -128, 127);
      int index = frame * FREQ_BINS + bin;
      input_data[index] = quantized;
    }
  }
  DBG_PRINTLN();
  DBG_PRINTLN("\nInput data:");
  for (int i = 0; i < 600; i++) {
    DBG_PRINT(input_data[i]);
    if (i % 50 == 49) {
      DBG_PRINTLN();
    } else {
      DBG_PRINT(", ");
    }
  }
  DBG_PRINTLN();
}

inline uint8_t quantize(float value, float scale, int zero) {
  return constrain(round((value / scale) + zero), -128, 127);
}

inline float dequantize(uint8_t value, float scale, int zero) {
  return (value - zero) * scale;
}

void preprocessAudio(int16_t* inputBuffer,
                     size_t inputBufferSize,
                     TfLiteTensor* modelInput) {
  if (inputBuffer == NULL || modelInput == NULL) {
    return;
  }

  float scale = modelInput->params.scale;
  int zero_point = modelInput->params.zero_point;

  int8_t* result = modelInput->data.int8;
  size_t resultSize = modelInput->bytes;
  size_t resultIndex = 0;

  while (inputBufferSize > 0) {
    size_t num_samples_read;
    struct FrontendOutput output = FrontendProcessSamples(
        &frontend_state, inputBuffer, inputBufferSize, &num_samples_read);
    inputBuffer += num_samples_read;
    inputBufferSize -= num_samples_read;

    if (output.values != NULL) {
      for (size_t i = 0; i < output.size && resultIndex < resultSize;
           i++, resultIndex++) {
        result[resultIndex] = quantize(output.values[i], scale, zero_point);
      }
    }
  }
}

bool runInference() {
  TfLiteTensor* input = interpreter->input(0);
  DBG_PRINTLN("Input shape");
  DBG_PRINTLN(input->dims->data[0]);
  DBG_PRINTLN(input->dims->data[1]);
  DBG_PRINTLN(input->dims->data[2]);
  DBG_PRINTLN(input->dims->data[3]);
  // computeQuantizedSpectrogram(input);
  preprocessAudio(recordBuffer, RECORD_BUFFER_SIZE, input);
  if (interpreter->Invoke() != kTfLiteOk) {
    DBG_PRINTLN("Inference failed!");
    return false;
  }

  TfLiteTensor* output = interpreter->output(0);
  DBG_PRINT("Output scale: ");
  DBG_PRINTLN(output->params.scale, 6);
  DBG_PRINT("Output zero point: ");
  DBG_PRINTLN(output->params.zero_point);

  // Example for classification output
  int8_t* output_data = output->data.int8;
  float scale = output->params.scale;
  int zero_point = output->params.zero_point;

  int num_classes = output->dims->data[1];

  float scores[num_classes];
  float max_score = -INFINITY;

  // Dequantize scores
  for (int i = 0; i < num_classes; i++) {
    scores[i] = dequantize(output_data[i], scale, zero_point);
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

  DBG_PRINTLN("Class probabilities:");
  for (int i = 0; i < num_classes; i++) {
    float probability = scores[i] / sum_exp;
    DBG_PRINT("Class ");
    DBG_PRINT(i);
    DBG_PRINT(": ");
    DBG_PRINTLN(probability, 2);

    if (probability > best_prob) {
      best_prob = probability;
      best_index = i;
    }
  }

  return true;
}