#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"
#include "./include/led.h"

uint32_t now;

void setup() {
  setupInit();
  setupServer(onWsEvent);
  setupLed();
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
  loopLed(now);
}