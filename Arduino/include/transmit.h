// transmit.h
// This module handles secure transmission of encrypted data.
// For now only over Serial, BLE is to be implemented.

#ifndef __TRANSMIT_H__
#define __TRANSMIT_H__

#include <Arduino.h>

#define SERIAL_BAUD_RATE    115200
#define HEADER_SIZE         2

void initTransmit();
bool transmitSerial(const byte* message);
bool transmitStringSerial(const char* message);

#endif