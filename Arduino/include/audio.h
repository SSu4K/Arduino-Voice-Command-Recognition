// audio.h 
// This component responisible for recieving and 
// recording raw audio data from the built in microphone.

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <math.h>
#include <PDM.h>

#define SAMPLE_BUFFER_SIZE  (128)

#define CHANNELS            int(1)
#define FREQUENCY           uint32_t(16000)

// We assume there always is just one channel
#define RECORD_SECONDS      (1)
#define RECORD_BUFFER_SIZE  (RECORD_SECONDS*FREQUENCY)

#define LOUDNESS_THRESHOLD  (300)

extern short recordBuffer[RECORD_BUFFER_SIZE];

void initPDM();
void onPDMData();
float measureLoudness();
void startRecording();
void stopRecording();
void recordSamples();
bool isRecordBufferFull();

#endif