// processing.h
// This module handles all processing of the audio data.
// Includes preprocessing and model inference.

#ifndef __PROCESSING_H__
#define __PROCESSING_H__

#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>

bool initModel();
bool initPreprocessor();
bool runInference();

#endif