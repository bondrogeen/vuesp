#include "./include/device.h"

#define GPIO_LED 13
#define GPIO_RELAY 14
#define GPIO_OUT_BTN 12
#define GPIO_IN_BTN 5

uint32_t lastTimeDevice = 0;
uint8_t btnStatus = 0;
Relay relay = {RELAY, 0, 0, 0};
uint32_t debounce;

void ICACHE_RAM_ATTR btnIsr() {
  btnStatus = 1;
}

void sendStatus() {
  uint8_t value = !digitalRead(GPIO_LED);
  digitalWrite(GPIO_LED, value);
  relay.status = value;
  wsSend((uint8_t *)&relay, sizeof(relay));
  attachInterrupt(GPIO_OUT_BTN, btnIsr, RISING);
  attachInterrupt(GPIO_IN_BTN, btnIsr, FALLING);
}

void setupDevice() {
  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_RELAY, OUTPUT);
  pinMode(GPIO_OUT_BTN, INPUT_PULLUP);
  pinMode(GPIO_IN_BTN, INPUT_PULLUP);
  attachInterrupt(GPIO_OUT_BTN, btnIsr, RISING);
  attachInterrupt(GPIO_IN_BTN, btnIsr, FALLING);
}

void loopDevice(uint32_t now) {
  if (btnStatus) {
    btnStatus = 0;
    detachInterrupt(GPIO_OUT_BTN);
    detachInterrupt(GPIO_IN_BTN);
    Serial.println(btnStatus);
    sendStatus();
  }

  if (now - lastTimeDevice > 1000) {
    lastTimeDevice = now;

    // digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    // digitalWrite(GPIO_RELAY, !digitalRead(GPIO_RELAY));
    Serial.println("sdsdsds");
  }
}
