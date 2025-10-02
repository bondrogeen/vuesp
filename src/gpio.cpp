#include "./include/gpio.h"

#include "./include/device.h"
#include "./include/files.h"
#include "./include/init.h"
#include "./include/tasks.h"

Port ports[5] = {
    {KEY_PORT, 4, INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 5, OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_OFF, GPIO_COMMAND_GET},
    {KEY_PORT, 12, OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 13, OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 14, OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
};

int ports_len = sizeof(ports) / sizeof(ports[0]);

volatile uint8_t btnStatus = 0;
uint32_t debounce = 0;

void ICACHE_RAM_ATTR btnIsr() {
  btnStatus = 1;
}

void initGPIO() {
  for (uint8_t i = 0; i < ports_len; i++) {
    port = ports[i];
    if (port.mode) {
      pinMode(port.gpio, port.mode);
      Serial.println(port.value);
      if (port.mode == OUTPUT || port.mode == OUTPUT_OPEN_DRAIN) digitalWrite(port.gpio, port.value);
    }
    ports[i].value = digitalRead(ports[i].gpio);
    if ((port.mode == INPUT || port.mode == INPUT_PULLUP) && port.interrupt) {
      attachInterrupt(port.gpio, btnIsr, port.interrupt);
    }
  }
}

void getAll() {
  for (int i = 0; i < ports_len; i++) {
    ports[i].value = digitalRead(ports[i].gpio);
    sendAll((uint8_t*)&ports[i], sizeof(ports[i]), KEY_PORT);
  }
}

void setMode() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == port.gpio) {
      ports[i] = port;
    }
  }
  sendAll((uint8_t*)&port, sizeof(port), KEY_PORT);
}

void setValue() {
  if (!(port.mode == INPUT || port.mode == INPUT_PULLUP)) digitalWrite(port.gpio, port.value);
  port.value = digitalRead(port.gpio);
  setMode();
}

void checkInterrupt() {
  for (int i = 0; i < ports_len; i++) {
    port = ports[i];
    if (!(port.mode == INPUT || port.mode == INPUT_PULLUP) && !port.interrupt) continue;

    port.value = digitalRead(port.gpio);
    if (port.interrupt == GPIO_INTERRUPT_CHANGE && ports[i].value != port.value) {
      ports[i].value = port.value;
      deviceGPIO(&port);
    }
    sendAll((uint8_t*)&port, sizeof(port), KEY_PORT);
  }
}

void setupFirstGPIO() {
  getLoadDef(DEF_PATH_GPIO, (uint8_t*)ports, sizeof(ports));
  initGPIO();
}

void loopGPIO(uint32_t now) {
  if (btnStatus == 1) {
    btnStatus = 2;
    debounce = now;
  }
  if (btnStatus == 2 && now - debounce > 50) {
    btnStatus = 0;
    checkInterrupt();
    deviceGPIOInterrupt();
  }

  if (tasks[KEY_PORT]) {
    Serial.println(tasks[KEY_PORT]);
    if (port.command == GPIO_COMMAND_SAVE) writeFile(DEF_PATH_GPIO, (uint8_t*)ports, sizeof(ports));
    if (port.command == GPIO_COMMAND_SET) setValue();
    if (port.command == GPIO_COMMAND_GET_ALL) getAll();
    if (port.command == GPIO_COMMAND_CHANGE) setMode();

    tasks[KEY_PORT] = 0;
  };
}
