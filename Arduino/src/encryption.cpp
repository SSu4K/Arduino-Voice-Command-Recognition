#include "encryption.h"
#include "debug.h"
#include <AES.h>
#include <CTR.h>
#include <Crypto.h>
#include <RNG.h>

CTR<AES128> ctr;

// example key
const byte key[KEY_SIZE] = {
  0x60, 0x3d, 0xeb, 0x10,
  0x15, 0xca, 0x71, 0xbe,
  0x2b, 0x73, 0xae, 0xf0,
  0x85, 0x7d, 0x77, 0x81
};

void initEncryption(){
    pinMode(A0, INPUT);
//   RNG.begin("manual");
//   byte seed[32];
//   for (int i = 0; i < 32; i++) {
//     seed[i] = analogRead(A0) & 0xFF;  // collect analog noise
//     delay(1);
//   }
//   RNG.seed(seed, sizeof(seed));
    DBG_PRINTLN("Encryption initialized!");
}

void generateRandomIV(byte *iv) {
  if(iv == NULL){
    return;
  }
  for (int i = 0; i < IV_SIZE; i++) {
    iv[i] = analogRead(A0) & 0xFF;
    delay(1);
  }
}

bool encrypt(const byte * message, size_t messageLength, byte * iv, byte * encrypted){
  if(message == NULL || iv == NULL || encrypted == NULL){
    return false; // null input parameters
  }
  
  if(messageLength > MAX_MESSAGE_SIZE){
    return false; // message too long
  }

  generateRandomIV(iv);
  ctr.setKey(key, KEY_SIZE);
  ctr.setIV(iv, IV_SIZE);

  ctr.encrypt(encrypted, message, messageLength);
  return true;
}