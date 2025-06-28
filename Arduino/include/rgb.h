// rgb.h
// This module handles basic color control of the RGB diode.

#ifndef __RGB_H__
#define __RGB_H__

typedef enum{
    Black,
    Red,
    Green,
    Yellow,
    Blue,
    Purple,
    Cyan,
    White
} Color;

bool initRGB();
void setColor(Color color);

#endif