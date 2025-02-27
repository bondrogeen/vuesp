#include "./include/gpio.h"

#include "./include/device.h"
#include "./include/files.h"
#include "./include/init.h"
#include "./include/tasks.h"

uint8_t gpio[5] = {4, 5, 12, 13, 14};
uint8_t ports[sizeof(gpio)][2] = {};

uint8_t btnStatus = 0;
uint32_t debounce = 0;

void ICACHE_RAM_ATTR btnIsr() {
  btnStatus = 1;
}

void changeBit(uint8_t *address, uint8_t value, uint8_t mask) {
  *address = value ? *address | mask : *address & ~mask;
}

uint8_t readBit(uint8_t data, uint8_t mask) {
  return bool(data & mask);
}

void defPorts() {
  for (uint8_t i = 0; i < sizeof(gpio); i++) {
    ports[i][0] = gpio[i];
    ports[i][1] = 0;
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

void initGpio() {
  for (uint8_t i = 0; i < sizeof(gpio); i++) {
    uint8_t data = ports[i][1];

    uint8_t init = (data & GPIO_INIT) >> 7;
    uint8_t mode = (data & GPIO_MODE) >> 4;
    uint8_t value = (data & GPIO_VALUE) >> 0;

    if (init) {
      pinMode(gpio[i], mode);
      digitalWrite(gpio[i], value);
    }
    if (mode == INPUT_PULLUP) {
      attachInterrupt(gpio[i], btnIsr, CHANGE);
    }
  }
}

void setupFirstGPIO() {
  getLoadDef(DEF_PATH_GPIO, (uint8_t *)ports, sizeof(ports));
}

void setupGPIO() {
  defPorts();
  initGpio();
}

uint8_t readPort(uint8_t port) {
  uint8_t value = digitalRead(port);
  return value;
}

void getAll(uint8_t readAll) {
  for (int i = 0; gpio[i]; i++) {
    port.gpio = gpio[i];
    uint8_t value = digitalRead(gpio[i]);
    uint8_t *address = &ports[i][1];
    changeBit(address, value, GPIO_VALUE);
    port.data = ports[i][1];
    if (readAll) {
      send((uint8_t *)&port, sizeof(port), KEY_PORT);
    } else {
      if (readBit(port.data, GPIO_VALUE_OLD) != value) {
        changeBit(address, value, GPIO_VALUE_OLD);
        send((uint8_t *)&port, sizeof(port), KEY_PORT);
      }
    }
  }
}

void loopGPIO(uint32_t now) {
  if (btnStatus == 1) {
    btnStatus = 2;
    debounce = now;
  }
  if (btnStatus == 2 && now - debounce > 50) {
    btnStatus = 0;
    getAll(false);
    deviceGPIO();
  }

  if (tasks[KEY_PORT]) {
    if (port.command == GPIO_COMMAND_GET_ALL) {
      getAll(true);
    }
    if (port.command == GPIO_COMMAND_SET) {
      digitalWrite(port.gpio, readBit(port.data, GPIO_VALUE));
      getAll(true);
    }
    tasks[KEY_PORT] = 0;
  };
}
