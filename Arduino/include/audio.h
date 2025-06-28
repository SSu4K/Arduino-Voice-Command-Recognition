// audio.h 
// This component responisible for recieving and 
// recording raw audio data from the built in microphone.

#ifndef __AUDIO_H__
#define __AUDIO_H__

#include <math.h>
#include <PDM.h>

#define SAMPLE_BUFFER_SIZE  (128)

#define CHANNELS            (1)
#define SAMPLE_RATE         (16000)

// We assume there always is just one channel
#define RECORD_SECONDS      (1)
#define RECORD_BUFFER_SIZE  (RECORD_SECONDS*SAMPLE_RATE)

#define GAIN                (1)
#define PREGAIN_MAX         (INT16_MAX/GAIN)
#define PREGAIN_MIN         (INT16_MIN/GAIN)

#define LOUDNESS_THRESHOLD  (200)

extern short recordBuffer[RECORD_BUFFER_SIZE];

bool initPDM();
void onPDMData();
float measureLoudness();
void startRecording();
void stopRecording();
void recordSamples();
bool isRecordBufferFull();

#endif