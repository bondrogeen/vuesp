#include "./gpio.h"

#if defined(ESP8266)
Port ports[6] = {
    {KEY_PORT, 4, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 5, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 12, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 13, GPIO_MODE_PWM, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 14, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 17, GPIO_MODE_ADC, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_OFF, GPIO_COMMAND_GET},
};
#elif defined(ARDUINO_ESP32C3_DEV)
Port ports[5] = {
    {KEY_PORT, 0, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 1, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 1, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 3, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 1, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 4, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 5, GPIO_MODE_PWM, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
};
#elif defined(ESP32)
Port ports[5] = {
    {KEY_PORT, 4, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 5, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_OFF, GPIO_COMMAND_GET},
    {KEY_PORT, 12, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 13, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
    {KEY_PORT, 14, GPIO_MODE_PWM, GPIO_INTERRUPT_OFF, 0, GPIO_STATE_ON, GPIO_COMMAND_GET},
};
#endif

OneWire* ds = nullptr;

Dallas ht1 = {KEY_DALLAS};

int ports_len = sizeof(ports) / sizeof(ports[0]);

volatile uint8_t btnStatus = 0;
uint32_t debounce = 0;
uint32_t lastTimeGPIO = 0;
uint32_t lastTimeADC = 0;
uint32_t isOneWire = 0;
uint32_t isADC = 0;

void ICACHE_RAM_ATTR btnIsr() {
  btnStatus = 1;
}

void initGPIO() {
  for (uint8_t i = 0; i < ports_len; i++) {
    port = ports[i];
    if (port.mode == GPIO_MODE_ONEWIRE) {
      if (!isOneWire) {
        if (ds) delete ds;
        ds = new OneWire(port.gpio);
        isOneWire = 1;
      }
      continue;
    }
    if (port.mode == GPIO_MODE_PWM) {
      pinMode(port.gpio, OUTPUT);
      analogWrite(port.gpio, port.value);
    } else if (port.mode == GPIO_MODE_ADC) {
      pinMode(port.gpio, INPUT);
      port.value = analogRead(port.gpio);
      isADC = 1;
    } else {
      pinMode(port.gpio, port.mode);
      if (port.mode == OUTPUT || port.mode == OUTPUT_OPEN_DRAIN) digitalWrite(port.gpio, port.value);

      ports[i].value = digitalRead(ports[i].gpio);
      if ((port.mode == INPUT || port.mode == INPUT_PULLUP) && port.interrupt) {
        attachInterrupt(port.gpio, btnIsr, port.interrupt);
      }
    }
  }
}

void getAll() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].mode == GPIO_MODE_PWM || ports[i].mode == GPIO_MODE_ONEWIRE) continue;
    if (ports[i].mode == GPIO_MODE_ADC) {
      ports[i].value = analogRead(ports[i].gpio);
    } else {
      ports[i].value = digitalRead(ports[i].gpio);
    }
    wsSendAll((uint8_t*)&ports[i], sizeof(ports[i]));
  }
}

void getADC() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].mode == GPIO_MODE_ADC) {
      ports[i].value = analogRead(ports[i].gpio);
      wsSendAll((uint8_t*)&ports[i], sizeof(ports[i]));
    }
  }
}

void updatePort() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == port.gpio) {
      ports[i] = port;
    }
  }
  wsSendAll((uint8_t*)&port, sizeof(port));
}

void updatePort(uint8_t gpio, uint16_t value) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      ports[i].value = value;
      port = ports[i];
    }
  }
  wsSendAll((uint8_t*)&port, sizeof(port));
}

void setValue(uint8_t gpio, uint16_t value, uint8_t mode) {
  if (mode == GPIO_MODE_PWM) {
    analogWrite(gpio, value);
  } else {
    if (!(mode == INPUT || mode == INPUT_PULLUP)) digitalWrite(gpio, value);
  }
}

void setValue(uint8_t gpio, uint16_t value) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      setValue(gpio, value, ports[i].mode);
      break;
    }
  }
}

uint16_t getValue(uint8_t gpio, uint16_t& value) {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      value = ports[i].value;
      return true;
    }
  }
  return false;
}

void setValueUpdate() {
  setValue(port.gpio, port.value, port.mode);
  port.value = digitalRead(port.gpio);
  updatePort();
}

void checkInterrupt() {
  for (int i = 0; i < ports_len; i++) {
    port = ports[i];
    if (!(port.mode == INPUT || port.mode == INPUT_PULLUP) && !port.interrupt) continue;

    port.value = digitalRead(port.gpio);
    if (port.interrupt == GPIO_INTERRUPT_CHANGE && ports[i].value != port.value) {
      ports[i].value = port.value;
      deviceGPIO(&port);
      char buffer[16];
      snprintf(buffer, sizeof(buffer), "btn_%d", port.gpio);
      scriptRunner.emitEvent(buffer);
      snprintf(buffer, sizeof(buffer), "btn_%d_%d", port.gpio, port.value);
      scriptRunner.emitEvent(buffer);
    }
    wsSendAll((uint8_t*)&port, sizeof(port));
  }
}

float getTemperature(uint8_t* address1) {
  if (!ds) return -127.0f;
  uint16_t temp;
  ds->reset();
  ds->select(address1);
  ds->write(0xBE);
  temp = (ds->read() | ds->read() << 8);

  ds->reset();
  ds->select(address1);
  ds->write(0x44, 1);
  return (float)temp / 16.0;
}

void findDallas() {
  if (!ds) return;
  while (ds->search(ht1.address) == 1) {
    ht1.temp = getTemperature(ht1.address);
    wsSendAll((uint8_t*)&ht1, sizeof(ht1));
  }
}

void stateChangeProvider(uint8_t gpio, uint16_t oldValue, uint16_t newValue) {
  updatePort(gpio, newValue);
}

bool portProvider(uint8_t gpio, PortAction action, uint16_t& value) {
  switch (action) {
    case PORT_READ:
      return getValue(gpio, value);
    case PORT_WRITE:
      setValue(gpio, value);
      return true;
  }
  return false;
}

void setupGPIO() {
  scriptRunner.setStateChangeProvider(stateChangeProvider);
  scriptRunner.setPortProvider(portProvider);
}

void setupFirstGPIO() {
  // getLoadDef(DEF_PATH_GPIO, (uint8_t*)ports, sizeof(ports));
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
  }

  if (now - lastTimeGPIO > 10000) {
    lastTimeGPIO = now;
    if (isOneWire) {
      findDallas();
    }
  }

  if (now - lastTimeADC > 1000 && isADC) {
    lastTimeADC = now;
    getADC();
  }

  if (tasks[KEY_PORT]) {
    if (port.command == GPIO_COMMAND_SAVE) writeFile(DEF_PATH_GPIO, (uint8_t*)ports, sizeof(ports));
    if (port.command == GPIO_COMMAND_SET) setValueUpdate();

    if (port.command == GPIO_COMMAND_GET_ALL) getAll();
    if (port.command == GPIO_COMMAND_CHANGE) updatePort();
    port.command = 0;
    tasks[KEY_PORT] = 0;
  };
}
