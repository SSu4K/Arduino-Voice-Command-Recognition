#include "audio.h"
#include "debug.h"

short sampleBuffer[SAMPLE_BUFFER_SIZE];
volatile int samplesRead;

short recordBuffer[RECORD_BUFFER_SIZE];
size_t recordBufferIndex = 0;

bool initPDM() {
  PDM.setBufferSize(SAMPLE_BUFFER_SIZE);
  PDM.onReceive(onPDMData);

  if (!PDM.begin(CHANNELS, SAMPLE_RATE)) {
    DBG_PRINTLN("Failed to start PDM!");
    return false;
  }

  DBG_PRINTLN("Microphone initialized.");
  return true;
}

void onPDMData() {
  int bytesAvailable = PDM.available();
  PDM.read(sampleBuffer, bytesAvailable);
  samplesRead = bytesAvailable / 2;
}

float measureLoudness() {
  if (samplesRead == 0) {
    return 0;
  }
  float sum = 0;
  for (int i = 0; i < samplesRead; i++) {
    sum += (sampleBuffer[i] * sampleBuffer[i]);
  }
  float rms = sqrt(sum / samplesRead);
  samplesRead = 0;
  return rms;
}

void startRecording() {
  recordBufferIndex = 0;
  DBG_PRINT("Started recording ");
  DBG_PRINT(RECORD_SECONDS);
  DBG_PRINTLN("s of audio.");
}

void stopRecording() {
  DBG_PRINTLN("Stopped recording.");
}

void recordSamples() {
  if (samplesRead == 0)
    return;
  for (int i = 0; i < samplesRead && !isRecordBufferFull();
       i++, recordBufferIndex++) {
    
    if(sampleBuffer[i] > PREGAIN_MAX){
        recordBuffer[recordBufferIndex] = INT16_MAX;
    }
    else if(sampleBuffer[i] < PREGAIN_MIN){
        recordBuffer[recordBufferIndex] = INT16_MIN;
    }
    else{
        recordBuffer[recordBufferIndex] = GAIN*sampleBuffer[i];
    }
  }
  // There might be some leftover samples after filling up the recordBuffer but
  // we don't care about them.
  samplesRead = 0;
}

bool isRecordBufferFull() {
  return recordBufferIndex >= RECORD_BUFFER_SIZE;
}