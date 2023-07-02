#ifndef DEVICE_H
#define DEVICE_H

#define NUMPIXELS 256

#include <Adafruit_NeoPixel.h>
#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupDevice();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif