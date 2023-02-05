#ifndef LED_H
#define LED_H

#include "./init.h"
#include <Adafruit_NeoPixel.h>
#define PIN D5

void setupLed();
void loopLed(uint32_t now);
void colorWipe(uint32_t c, uint8_t wait);

#endif