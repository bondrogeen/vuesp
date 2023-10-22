#ifndef DEVICE_H
#define DEVICE_H

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupDevice();
void eventGPIO(uint8_t port, uint8_t value);
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif