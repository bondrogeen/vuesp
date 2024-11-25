#ifndef DEVICE_H
#define DEVICE_H

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

#define GPIO_SDA 4
#define GPIO_SCL 15

#define GPIO_ADC1 36
#define GPIO_ADC2 39
#define GPIO_ADC3 34
#define GPIO_ADC4 35

#define ADDRESS_OUTPUT 0x24
#define ADDRESS_INPUT 0x22

void setupDevice();
void getGPIO();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif