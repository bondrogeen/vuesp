#ifndef LED_H
#define LED_H

#include <Adafruit_NeoPixel.h>

#include "./include/device.h"

#define NUMPIXELS 256
#define GPIO_LED 14  // D5

#define MATRIX_TYPE (0U)
#define SEGMENTS (1U)

#define WIDTH 16
#define HEIGHT 16

#define LIGHTERS_AM (100U)
#define SPARKLES (1U)

void setupLed();
void led(Device device);
void ledShow();
void ledClear();
void ledBrightness(uint8_t brightness);

void ledEffectLighters();
void ledEffectFire();
void rainbow_fade();

#endif