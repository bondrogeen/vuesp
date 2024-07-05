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

void setupDevice();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);
void eventGPIO(uint8_t port, uint8_t value);

#endif