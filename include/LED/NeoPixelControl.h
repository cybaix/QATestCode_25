#ifndef NEOPIXEL_CONTROL_H
#define NEOPIXEL_CONTROL_H

#include <Adafruit_NeoPixel.h>

void initNeoPixels();
void setNeoPixelColor(uint16_t n, uint32_t color);
void clearNeoPixels();
void cycleNeoPixelColors();  // New function to cycle colors

#endif // NEOPIXEL_CONTROL_H
