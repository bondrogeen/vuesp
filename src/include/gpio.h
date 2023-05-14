#ifndef GPIO_H
#define GPIO_H

#include "./init.h"
#include "./webserver.h"
#include "./tasks.h"

void setupGPIO();
void loopGPIO(uint32_t now);

#endif