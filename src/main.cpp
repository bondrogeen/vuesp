#include "init.h"
#include "led.h"
#include "tasks.h"
#include "webserver.h"

uint32_t now;

void setup() {
  initApp();
  initServer(onWsEvent);
  setupLed();
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
  loopLed(now);
}