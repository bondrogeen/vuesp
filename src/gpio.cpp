#include "./include/gpio.h"

#include "./include/device.h"
#include "./include/files.h"
#include "./include/init.h"
#include "./include/tasks.h"

Port ports[5] = {
    {KEY_PORT, 4, INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 5, OUTPUT, GPIO_INTERRUPT_FALLING, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 12, OUTPUT, GPIO_INTERRUPT_OFF, 1, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 13, OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 14, OUTPUT, GPIO_INTERRUPT_OFF, 1, GPIO_STATE_ON, GPIO_COMMAND_GET},
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
    port = ports[i];

    if (port.mode) {
      pinMode(port.gpio, port.mode);
      if (port.mode != INPUT || port.mode != INPUT_PULLUP) digitalWrite(port.gpio, port.value);
    }
    if (port.mode == INPUT || port.mode == INPUT_PULLUP) {
      attachInterrupt(port.gpio, btnIsr, port.interrupt);
    }
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
    port = ports[i];
    uint8_t value = digitalRead(port.gpio);
    if (port.value != value) {
      Serial.println(port.gpio);
      // port.value = value;
      // deviceGPIO(&port);
    }
    port.value = value;
    if (readAll) {
      send((uint8_t *)&port, sizeof(port), KEY_PORT);
    } else {
      // if (port.mode != INPUT || port.mode != INPUT_PULLUP) continue;
      if (port.interrupt) {
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
    Serial.println("debounce");
    getAll(false);
  }

  if (tasks[KEY_PORT]) {
    if (port.command == GPIO_COMMAND_SET) {
      digitalWrite(port.gpio, port.value);
      getAll(true);
    }
    if (port.command == GPIO_COMMAND_GET_ALL) {
      getAll(true);
    }
    tasks[KEY_PORT] = 0;
  };
}
