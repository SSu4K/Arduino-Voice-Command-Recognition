#include "transmit.h"
#include "encryption.h"

const byte HEADER[HEADER_SIZE] = {0xAA, 0x55};

void initTransmit() {
  Serial.begin(SERIAL_BAUD_RATE);
  while (!Serial);
}

byte calculateChecksum(const byte* data, size_t length) {
    byte checksum = 0;
    for (size_t i = 0; i < length; i++) {
        checksum ^= data[i];
    }
    return checksum;
}

bool transmitSerial(const byte* message, size_t messageLength) {
  if (message == NULL) {
    return false;
  }
  if (messageLength > MAX_MESSAGE_SIZE) {
    return false;
  }

  static byte iv[IV_SIZE];
  static byte encrypted[MAX_MESSAGE_SIZE];

  if (encrypt(message, messageLength, iv, encrypted)) {
    Serial.write(HEADER, HEADER_SIZE); 
    Serial.write((byte)messageLength);
    Serial.write(iv, IV_SIZE);
    Serial.write(encrypted, messageLength);

    size_t checksumLen = 1 + IV_SIZE + messageLength;
    byte checksumData[1 + IV_SIZE + MAX_MESSAGE_SIZE];
    checksumData[0] = (byte)messageLength;
    memcpy(checksumData + 1, iv, IV_SIZE);
    memcpy(checksumData + 1 + IV_SIZE, encrypted, messageLength);

    byte checksum = calculateChecksum(checksumData, checksumLen);
    Serial.write(checksum);                

    Serial.flush();
    return true;
  }

  return false;
}

bool transmitStringSerial(const char* message) {
  if (message == NULL) {
    return false;
  }
  size_t messageLength = strlen(message);

  return transmitSerial((const byte*)message, messageLength);
}