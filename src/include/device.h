#ifndef DEVICE_H
#define DEVICE_H

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupDevice();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);
void eventGPIO(uint8_t port, uint8_t value);

#endif