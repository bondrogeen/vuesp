#ifndef DEVICE_H
#define DEVICE_H

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupDevice();
void loopDevice(uint32_t now);

#endif