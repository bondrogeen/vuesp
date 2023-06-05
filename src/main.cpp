#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"
#include "./include/led.h"
#include "./include/gpio.h"
#include "./include/device.h"

uint32_t now;

void setup() {
  setupInit();
  setupServer();
  setupGPIO();
  setupDevice();
  setupLed();
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
  loopGPIO(now);
  loopDevice(now);
  loopLed(now);
}