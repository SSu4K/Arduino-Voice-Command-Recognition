#include "rgb.h"
#include "Arduino.h"

static const bool DIODE_MAP[8][3] = {
    {1, 1, 1},
    {0, 1, 1},
    {1, 0, 1},
    {0, 0, 1},
    {1, 1, 0},
    {0, 1, 0},
    {1, 0, 0},
    {0, 0, 0},
};

bool initRGB(){
    pinMode(LEDR, OUTPUT);
    digitalWrite(LEDR, HIGH);
    pinMode(LEDG, OUTPUT);
    digitalWrite(LEDG, HIGH);
    pinMode(LEDB, OUTPUT);
    digitalWrite(LEDB, HIGH);
    return true;
}

void setColor(Color color){
    digitalWrite(LEDR, DIODE_MAP[color][0]);
    digitalWrite(LEDG, DIODE_MAP[color][1]);
    digitalWrite(LEDB, DIODE_MAP[color][2]);
}
