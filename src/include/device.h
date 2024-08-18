#ifndef DEVICE_H
#define DEVICE_H

#define GPIO_SDA 4
#define GPIO_SCL 5

#define GPIO_ADC1 36
#define GPIO_ADC2 34
#define GPIO_ADC3 35
#define GPIO_ADC4 39

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupDevice();
void eventGPIO(uint8_t port, uint8_t value);
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif