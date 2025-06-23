#ifndef __PROCESSING_H__
#define __PROCESSING_H__

#include <TensorFlowLite.h>
#include <tensorflow/lite/micro/micro_interpreter.h>
#include <tensorflow/lite/micro/all_ops_resolver.h>
#include <tensorflow/lite/schema/schema_generated.h>

void initModel();
void processData(byte const * data, size_t size);
bool runInference();

#endif