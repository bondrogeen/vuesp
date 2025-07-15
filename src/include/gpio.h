#ifndef GPIO_H
#define GPIO_H

#define GPIO_VALUE 0b00000001
#define GPIO_VALUE_OLD 0b00000010
#define GPIO_EMPTY 0b00001100
#define GPIO_MODE 0b01110000
#define GPIO_INIT 0b10000000

#define GPIO_COMMAND_GET 0
#define GPIO_COMMAND_SET 1
#define GPIO_COMMAND_GET_ALL 2

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupFirstGPIO();
void setupGPIO();
void loopGPIO(uint32_t now);

#endif