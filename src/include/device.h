#ifndef DEVICE_H
#define DEVICE_H

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

#define DEVICE_COMMAND_SAVE 254
#define DEVICE_COMMAND_GET 1
#define DEVICE_COMMAND_SET 2
#define DEVICE_COMMAND_TEXT 3

void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port);
void deviceGPIOInterrupt();
void loopDevice(uint32_t now);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif