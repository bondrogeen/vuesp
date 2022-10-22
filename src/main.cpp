#include "init.h"
#include "tasks.h"
#include "webserver.h"


uint32_t now;

void setup() {
  Serial.begin(115200);
  initApp();
  initServer(onWsEvent);
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
}