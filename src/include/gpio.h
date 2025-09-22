#ifndef GPIO_H
#define GPIO_H

#define GPIO_STATE_OFF 0
#define GPIO_STATE_ON 1

#define GPIO_INTERRUPT_OFF 0
#define GPIO_INTERRUPT_RISING 1
#define GPIO_INTERRUPT_FALLING 2
#define GPIO_INTERRUPT_CHANGE 3

#define GPIO_COMMAND_GET 0
#define GPIO_COMMAND_SET 1
#define GPIO_COMMAND_GET_ALL 2

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupFirstGPIO();
void loopGPIO(uint32_t now);

#endif