#include "./include/gpio.h"

#include "./include/init.h"
#include "./include/tasks.h"

#define GPIO_LED 13
#define GPIO_RELAY 14
#define GPIO_OUT_BTN 12
#define GPIO_IN_BTN 5

uint8_t gpio[5] = {4, 5, 12, 13, 14};
uint8_t ports[sizeof(gpio)][2] = {};

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

void changeBit(uint8_t *address, uint8_t value, uint8_t mask) {
  *address = value ? *address | mask : *address & ~mask;
}

uint8_t readBit(uint8_t data, uint8_t mask) {
  return bool(data & mask);
}

void defPorts() {
  for (uint8_t i = 0; i < sizeof(gpio); i++) {
    ports[i][0] = gpio[i];
    uint8_t *adress = &ports[i][1];

    changeBit(adress, 1, GPIO_INIT);
    changeBit(adress, INPUT, GPIO_MODE);
    changeBit(adress, 1, GPIO_STATUS);
  }
}

uint8_t getPort(uint8_t gpio) {
  for (uint8_t i = 0; i < sizeof(gpio); i++) {
    if (ports[i][0] == gpio) {
      return ports[i][1];
      break;
    }
  }
  return 0;
}

// uint8_t getarr() {
//   uint8_t arr[10];
//   for (uint8_t i = 0; ports[i]; i++) {
//     arr[i] = ports[i][0];
//     arr[i + 1] = ports[i][1];
//   }

//   return
// }

void setupGPIO() {
  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_RELAY, OUTPUT);

  uint8_t isOk = readFile("/gpio.io", (uint8_t *)ports, sizeof(ports));

  Serial.println(isOk);
  if (!isOk) {
    defPorts();
    writeFile("/gpio.io", (uint8_t *)ports, sizeof(ports));
  } else {
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

    // tasks[KEY_PORT] = 1;

    if (tasks[KEY_PORT]) {
      if (port.command == GPIO_COMMAND_GET_ALL) {
        for (int i = 0; gpio[i]; i++) {
          port.gpio = gpio[i];
          port.data = getPort(gpio[i]);
          send((uint8_t *)&port, sizeof(port), KEY_PORT);
        }
      }
      if (port.command == GPIO_COMMAND_SET) {
        digitalWrite(port.gpio, readBit(port.data, GPIO_VALUE));
        Serial.println(readBit(port.data, GPIO_VALUE));
        Serial.println(port.data);
      }
      Serial.println(port.gpio);
      // send((uint8_t *)&test, sizeof(test), KEY_PORT);
      // digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
      tasks[KEY_PORT] = 0;
    };

    // digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    // digitalWrite(GPIO_RELAY, !digitalRead(GPIO_RELAY));
    Serial.println("sdsdsds");
  }
}
