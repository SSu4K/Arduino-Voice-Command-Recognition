// encryption.h
// This component handles encryption using AES CTR.

#ifndef __ENCRYPTION_H__
#define __ENCRYPTION_H__

#include <Arduino.h>

// Never change these:
#define MAX_MESSAGE_SIZE    128
#define KEY_SIZE            16
#define IV_SIZE             KEY_SIZE

bool initEncryption();
bool encrypt(const byte * message, size_t messageLength, byte * iv, byte * encrypted);

#endif