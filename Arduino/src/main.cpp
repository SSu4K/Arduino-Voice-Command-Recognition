#include <Arduino.h>
#include <ArduinoJson.h>
#include <arduinoFFT.h>

#include "audio.h"
#include "processing.h"
#include "rgb.h"
#include "transmit.h"

typedef enum { IDLE, RECORDING, PROCESSING } Status;

Status boardState;

void idleStep();
void recordingStep();
void processingStep();

void setup() {
  delay(2000);  // Give USB CDC time to reconnect

  initRGB();
  initTransmit();
  initPDM();

  initModel();

  boardState = IDLE;
}

void loop() {
  switch (boardState) {
    case IDLE:
      idleStep();
      break;
    case RECORDING:
      recordingStep();
      break;
    case PROCESSING:
      processingStep();
      break;
  }
}

void idleStep() {
  float loudness = measureLoudness();

  if (loudness > LOUDNESS_THRESHOLD / 8) {
    setColor(Green);
  } else {
    setColor(Black);
  }

  if (loudness > LOUDNESS_THRESHOLD) {
    setColor(Red);
    boardState = RECORDING;
    startRecording();
  }
}
void recordingStep() {
  recordSamples();
  if (isRecordBufferFull()) {
    stopRecording();
    setColor(Black);
    boardState = PROCESSING;
  }
}

void processingStep() {
  setColor(Yellow);
  // processing code here in future
  // for now simulate processing time
  //   delay(500);

  //   long double sum = 0;
  //   for (size_t i = 0; i < RECORD_BUFFER_SIZE; i++) {
  //     sum += recordBuffer[i] * recordBuffer[i];
  //   }
  //   double avg = sqrt(sum / RECORD_BUFFER_SIZE);

  //   StaticJsonDocument<256> doc;
  //   doc["volume"] = avg;
  //   doc["timestamp"] = millis();  // example extra data

  //   char jsonBuffer[256];
  //   serializeJson(doc, jsonBuffer, sizeof(jsonBuffer));

  //   // Transmit encrypted
  //   if (transmitStringSerial(jsonBuffer)) {
  //     Serial.println("");
  //     Serial.print("Sent encrypted volume: ");
  //     Serial.println(jsonBuffer);
  //   } else {
  //     Serial.println("Failed to send encrypted volume.");
  //   }

//   for(size_t i = 0; i<RECORD_BUFFER_SIZE; i++){
//     Serial.println(recordBuffer[i]);
//   }
  
  runInference();
  boardState = IDLE;
  setColor(Black);
}