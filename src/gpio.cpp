#include "./include/gpio.h"

#include "./include/device.h"
#include "./include/files.h"
#include "./include/init.h"
#include "./include/tasks.h"

Port ports[5] = {
    {KEY_PORT, 0, 4, INPUT, 1, 0},
    {KEY_PORT, 0, 5, OUTPUT, 0, 0},
    {KEY_PORT, 0, 12, OUTPUT_OPEN_DRAIN, 1, 0},
    {KEY_PORT, 0, 13, OUTPUT, 0, 0},
    {KEY_PORT, 0, 14, OUTPUT, 0, 0},
};

int ports_len = sizeof(ports) / sizeof(ports[0]);

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

void initGpio() {
  for (uint8_t i = 0; i < ports_len; i++) {
    uint8_t pin = ports[i].gpio;
    uint8_t value = ports[i].value;
    uint8_t interrupt = ports[i].interrupt;
    uint8_t mode = ports[i].mode;

    // if (mode) {
    //   pinMode(pin, mode);
    //   digitalWrite(pin, value);
    // }
    // if (interrupt) {
    //   attachInterrupt(pin, btnIsr, interrupt);
    // }
  }
}

void setupFirstGPIO() {
  // uint8_t isOk = readFile(DEF_PATH_GPIO, (uint8_t *)ports, sizeof(ports));
  // if (!isOk) {

  writeFile(DEF_PATH_GPIO, (uint8_t *)ports, sizeof(ports));
  // }
  initGpio();
}

uint8_t readPort(uint8_t port) {
  uint8_t value = digitalRead(port);
  return value;
}

void getAll(uint8_t readAll) {
  for (int i = 0; i < ports_len; i++) {
    uint8_t pin = ports[i].gpio;
    port.gpio = pin;
    port.value = digitalRead(pin);

    if (readAll) {
      send((uint8_t *)&port, sizeof(port), KEY_PORT);
    } else {
      uint8_t interrupt = ports[i].interrupt;
      if (interrupt) {
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
      digitalWrite(port.gpio, port.value);
      getAll(true);
    }
    tasks[KEY_PORT] = 0;
  };
}
