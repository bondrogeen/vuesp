#ifndef DEVICE_H
#define DEVICE_H

#include <Wire.h>

#include "../libs/UnixTime.h"
#include "./gpio/gpio.h"
#include "./runner/runner.h"
#include "../include/led.h"

// LED matrix commands use their own numbering to stay compatible with the
// existing Draw protocol; generic device commands come from const/define.h.
#define DEVICE_COMMAND_GET_ALL 0
#define DEVICE_COMMAND_SET 1
#define DEVICE_COMMAND_GET 2
#define DRAW_COMMAND_DRAW 3
#define DRAW_COMMAND_SAVE 4

#define GPIO_BTN 13
#define GPIO_HLK_LD101 12
#define GPIO_SDA 0
#define GPIO_SCL 2

#define BRIGHTNESS 10
#define SPEED 60

#define EFFECT_DRAW 0
#define EFFECT_LIGHTERS 1
#define EFFECT_FIRE 2
#define EFFECT_RAINBOW 3
#define EFFECT_TWINKLING_STARS 4
#define EFFECT_CHASE 5

extern Device device;
extern Draw draw;
extern Sensors sensors;

void setupDevice();
void setupFirstDevice();
void deviceGPIO(Port* port, uint8_t type);
void loopDevice(uint32_t now);
void sendDeviceTo(uint32_t clientId);
void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task);

#endif
