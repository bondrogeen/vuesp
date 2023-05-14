#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"
#include "./include/gpio.h"

uint32_t now;

void setup() {
  setupInit();
  setupServer(onWsEvent);
  setupGPIO();
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
  loopGPIO(now);
}