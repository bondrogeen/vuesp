#ifndef GPIO_H
#define GPIO_H

#define GPIO_VALUE 0b00000001
#define GPIO_VALUE_OLD 0b00000010
#define GPIO_INTERRUPT 0b00001100
#define GPIO_MODE 0b01110000
#define GPIO_DISABLED 0b10000000

#define GPIO_MODE_INPUT (INPUT << 4)
#define GPIO_MODE_INPUT_PULLUP (INPUT_PULLUP << 4)
#define GPIO_MODE_OUTPUT (OUTPUT << 4)
#define GPIO_MODE_OUTPUT_OPEN_DRAIN (OUTPUT_OPEN_DRAIN << 4)

#define GPIO_INTERRUPT_RISING (RISING << 2)
#define GPIO_INTERRUPT_FALLING (FALLING << 2)
#define GPIO_INTERRUPT_CHANGE (CHANGE << 2)

#define GPIO_COMMAND_GET 0
#define GPIO_COMMAND_SET 1
#define GPIO_COMMAND_GET_ALL 2

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupFirstGPIO();
void loopGPIO(uint32_t now);

#endif