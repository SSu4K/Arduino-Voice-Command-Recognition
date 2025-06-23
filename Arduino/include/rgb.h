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

void initRGB();
void setColor(Color color);

#endif