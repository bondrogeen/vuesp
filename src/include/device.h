#ifndef DEVICE_H
#define DEVICE_H

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

#define GPIO_PWM 4
#define GPIO_SENSOR1 13
#define GPIO_SENSOR2 14
#define GPIO_18B20 12

void setupDevice();
void setupFirstDevice();
void getGPIO();
void deviceGPIO();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif