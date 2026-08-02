#ifndef DEVICE_H
#define DEVICE_H

#include <Wire.h>

#include "../libs/UnixTime.h"
#include "./gpio/gpio.h"
#include "./runner/runner.h"

#define GPIO_SDA 4
#define GPIO_SCL 15

#define GPIO_DAC1 25
#define GPIO_DAC2 26

#define GPIO_ADC1 36
#define GPIO_ADC2 39
#define GPIO_ADC3 34
#define GPIO_ADC4 35

#define GPIO_HT1 32

#define GPIO_INT 13

#define ADDRESS_RTC 0x68
#define ADDRESS_OUTPUT 0x24
#define ADDRESS_INPUT 0x22

void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port);
void loopDevice(uint32_t now);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif