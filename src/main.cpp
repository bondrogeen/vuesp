#include "./include/device.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"

uint32_t now;
uint8_t isInit = 0;

void setup() {
  setupInit();
  setupServer();
  setupDevice();
}

void loop() {
  now = millis();
  if (!isInit && now > 5000) {
    setupGPIO();
    isInit = true;
  }

  loopServer(now);
  loopTask(now);
  loopGPIO(now);
  loopDevice(now);
}