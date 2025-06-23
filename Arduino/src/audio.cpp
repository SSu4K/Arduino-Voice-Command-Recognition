#include "audio.h"

short sampleBuffer[SAMPLE_BUFFER_SIZE];
volatile int samplesRead;

short recordBuffer[RECORD_BUFFER_SIZE];
size_t recordBufferIndex = 0;

void initPDM() {
  PDM.setBufferSize(SAMPLE_BUFFER_SIZE);
  PDM.onReceive(onPDMData);

  if (!PDM.begin(CHANNELS, FREQUENCY)) {
    Serial.println("Failed to start PDM!");
    while (1)
      ;
  }

  Serial.println("Microphone initialized");
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
  Serial.print("Started recording ");
  Serial.print(RECORD_SECONDS);
  Serial.println("s of audio.");
}

void stopRecording() {
  Serial.println("Stopped recording.");
}

void recordSamples() {
  if (samplesRead == 0)
    return;
  for (int i = 0; i < samplesRead && !isRecordBufferFull();
       i++, recordBufferIndex++) {
    recordBuffer[recordBufferIndex] = sampleBuffer[i];
  }
  // There might be some leftover samples after filling up the recordBuffer but
  // we don't care about them.
  samplesRead = 0;
}

bool isRecordBufferFull() {
  return recordBufferIndex >= RECORD_BUFFER_SIZE;
}