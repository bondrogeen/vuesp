#ifndef GPIO_H
#define GPIO_H

#define GPIO_MODE_INPUT INPUT
#define GPIO_MODE_INPUT_PULLUP INPUT_PULLUP
#define GPIO_MODE_OUTPUT OUTPUT
#define GPIO_MODE_OUTPUT_OPEN_DRAIN OUTPUT_OPEN_DRAIN
#define GPIO_MODE_PWM 128

#define GPIO_STATE_OFF 0
#define GPIO_STATE_ON 1

#define GPIO_INTERRUPT_OFF 0
#define GPIO_INTERRUPT_RISING 1
#define GPIO_INTERRUPT_FALLING 2
#define GPIO_INTERRUPT_CHANGE 3

#define GPIO_COMMAND_SAVE 254
#define GPIO_COMMAND_GET 1
#define GPIO_COMMAND_SET 2
#define GPIO_COMMAND_GET_ALL 3
#define GPIO_COMMAND_CHANGE 4

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupFirstGPIO();
void loopGPIO(uint32_t now);

#endif