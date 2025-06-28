#ifndef DEVICE_H
#define DEVICE_H

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

#define COMMAND_GET_ALL 0
#define COMMAND_SET 1
#define COMMAND_GET 2
#define COMMAND_DRAW 3
#define COMMAND_SAVE 4

#define GPIO_BTN 13
#define GPIO_HLK_LD101 12
#define GPIO_SDA 0
#define GPIO_SCL 2

#define BRIGHTNESS 10
#define SPEED 60

#define EFFECT_DRAW 0
#define EFFECT_LIGHTERS 1
#define EFFECT_FIRE 2
#define EFFECT_TWINKLEUP 3

#define GPIO_SDA 4
#define GPIO_SCL 15

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
void deviceGPIO();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif