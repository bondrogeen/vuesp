#ifndef GPIO_H
#define GPIO_H

#define GPIO_INIT 1
#define GPIO_EMPTY_2 2
#define GPIO_EMPTY_3 4
#define GPIO_EMPTY_4 8
#define GPIO_EMPTY_5 16
#define GPIO_VALUE 32
#define GPIO_MODE 64
#define GPIO_STATUS 128

#define GPIO_COMMAND_GET 0
#define GPIO_COMMAND_SET 1
#define GPIO_COMMAND_GET_ALL 2

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupGPIO();
void loopGPIO(uint32_t now);

#endif