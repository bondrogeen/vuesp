#ifndef DEVICE_H
#define DEVICE_H

#include <Wire.h>

#include "../libs/UnixTime.h"
#include "./gpio/gpio.h"
#include "./runner/runner.h"

#define DEVICE_COMMAND_SAVE 254
#define DEVICE_COMMAND_GET 1
#define DEVICE_COMMAND_SET 2
#define DEVICE_COMMAND_TEXT 3

void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port);
void loopDevice(uint32_t now);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif