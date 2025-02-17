#ifndef DEVICE_H
#define DEVICE_H

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

#define GPIO_SDA 4
#define GPIO_SCL 15

#define GPIO_485TX 27
#define GPIO_485RX 14

#define GPIO_ADC1 36
#define GPIO_ADC2 39
#define GPIO_ADC3 34
#define GPIO_ADC4 35

#define GPIO_HT1 32

#define ADDRESS_RTC 0x68
#define ADDRESS_OUTPUT 0x24
#define ADDRESS_INPUT 0x22

void setupDevice();
void setupFirstDevice();
void getGPIO();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif