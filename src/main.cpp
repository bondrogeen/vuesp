#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"

uint32_t now;

Gpio btn = {GPIO, D1, 0, false};

unsigned long button_time = 0;
unsigned long last_button_time = 0;

void IRAM_ATTR isr() {
  button_time = millis();
  if (button_time - last_button_time > 250) {
    btn.presses++;
    btn.pressed = true;
    last_button_time = button_time;
  }
}

void setup() {
  setupInit();
  setupServer(onWsEvent);
  pinMode(btn.gpio, INPUT_PULLUP);
  attachInterrupt(btn.gpio, isr, FALLING);
  pinMode(D5, OUTPUT);
  pinMode(D6, OUTPUT);
  digitalWrite(D5, HIGH);
  digitalWrite(D6, HIGH);
}

void loop() {
  now = millis();
  loopServer(now);
  loopTask(now);
  if (btn.pressed) {
    btn.pressed = false;
    digitalWrite(D5, !digitalRead(D5));
    digitalWrite(D6, !digitalRead(D6));
    wsSend((uint8_t *)&btn, sizeof(btn));
  }
}