#include "./include/gpio.h"

#include "./include/init.h"
#include "./include/tasks.h"

#define GPIO_LED 13
#define GPIO_RELAY 14
#define GPIO_OUT_BTN 12
#define GPIO_IN_BTN 5

uint8_t gpio[5] = {4, 5, 12, 13, 14};
uint8_t ports[60] = {};

uint32_t lastTimeDevice = 0;
// uint8_t btnStatus = 0;
// Relay relay = {RELAY, 0, 0, 0};
// uint32_t debounce;

// void ICACHE_RAM_ATTR btnIsr() {
//   btnStatus = 1;
// }

// void sendStatus() {
//   uint8_t value = !digitalRead(GPIO_LED);
//   digitalWrite(GPIO_LED, value);
//   relay.status = value;
//   wsSend((uint8_t *)&relay, sizeof(relay));
//   attachInterrupt(GPIO_OUT_BTN, btnIsr, RISING);
//   attachInterrupt(GPIO_IN_BTN, btnIsr, FALLING);
// }

void setupGPIO() {
  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_RELAY, OUTPUT);

  uint8_t isOk = readFile("/gpio.io", ports, sizeof(ports));
  if (!isOk) {
    writeFile("/gpio.io", ports, sizeof(ports));
  }
  // pinMode(GPIO_OUT_BTN, INPUT_PULLUP);
  // pinMode(GPIO_IN_BTN, INPUT_PULLUP);
  // attachInterrupt(GPIO_OUT_BTN, btnIsr, RISING);
  // attachInterrupt(GPIO_IN_BTN, btnIsr, FALLING);
}

uint8_t readPort(uint8_t port) {
  uint8_t value = digitalRead(port);
  return value;
}

void loopGPIO(uint32_t now) {
  // if (btnStatus) {
  //   btnStatus = 0;
  //   detachInterrupt(GPIO_OUT_BTN);
  //   detachInterrupt(GPIO_IN_BTN);
  //   Serial.println(btnStatus);
  //   sendStatus();
  // }

  if (now - lastTimeDevice > 1000) {
    lastTimeDevice = now;

    tasks[KEY_PORT] = 1;

    if (tasks[KEY_PORT]) {
      for (int i = 0; gpio[i]; i++) {
        ports[gpio[i]] = 134;
      }
      writeFile("/gpio.io", ports, sizeof(ports));
      // send((uint8_t *)&test, sizeof(test), KEY_PORT);
    };

    digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    digitalWrite(GPIO_RELAY, !digitalRead(GPIO_RELAY));
    Serial.println("sdsdsds");
  }
}
