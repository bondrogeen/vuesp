#include "./include/device.h"
#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"

uint32_t now;

void setup() {
  setupInit();
  setupServer(onWsEvent);
  setupDevice();
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
  loopDevice(now);
}