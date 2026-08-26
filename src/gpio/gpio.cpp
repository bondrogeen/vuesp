#include "./gpio.h"

#if defined(ESP8266)
Port ports[6] = {
    {KEY_PORT, 4, 0, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, GPIO_BIT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF, GPIO_EVENT_ON},
    {KEY_PORT, 5, 0, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, GPIO_BIT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF, GPIO_EVENT_ON},
    {KEY_PORT, 12, 0, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 13, 255, GPIO_MODE_PWM, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 14, 0, GPIO_MODE_PWM, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC_AND_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 17, 0, GPIO_MODE_ADC, GPIO_INTERRUPT_OFF, GPIO_BIT_ADC, GPIO_DISABLED_ON},
};
#elif defined(ARDUINO_ESP32C3_DEV)
Port ports[5] = {
    {KEY_PORT, 0, 0, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 1, 0, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 3, 0, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 4, 0, GPIO_MODE_OUTPUT, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 5, 0, GPIO_MODE_PWM, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
};
#elif defined(ESP32)
Port ports[10] = {
    {KEY_PORT, 13, 0, GPIO_MODE_INPUT_PULLUP, GPIO_INTERRUPT_CHANGE, GPIO_BIT_INPUT_PULLUP, GPIO_DISABLED_ON},
    {KEY_PORT, 12, 0, GPIO_MODE_INPUT, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 25, 0, GPIO_MODE_DAC, GPIO_INTERRUPT_OFF, GPIO_BIT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 26, 0, GPIO_MODE_DAC, GPIO_INTERRUPT_OFF, GPIO_BIT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 32, 0, GPIO_MODE_ONEWIRE, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 33, 0, GPIO_MODE_INPUT, GPIO_INTERRUPT_OFF, GPIO_BIT_ALL_NOT_DAC, GPIO_DISABLED_OFF},
    {KEY_PORT, 34, 0, GPIO_MODE_ADC, GPIO_INTERRUPT_OFF, GPIO_BIT_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 35, 0, GPIO_MODE_ADC, GPIO_INTERRUPT_OFF, GPIO_BIT_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 36, 0, GPIO_MODE_ADC, GPIO_INTERRUPT_OFF, GPIO_BIT_ADC, GPIO_DISABLED_OFF},
    {KEY_PORT, 39, 0, GPIO_MODE_ADC, GPIO_INTERRUPT_OFF, GPIO_BIT_ADC, GPIO_DISABLED_OFF},
};
#endif

OneWire* ds = nullptr;

Dallas ht1 = {KEY_DALLAS};

int ports_len = sizeof(ports) / sizeof(ports[0]);

volatile uint8_t btnStatus = 0;
uint32_t debounce = 0;
uint32_t lastLoopInterrupt = 0;
uint32_t lastTimeGPIO = 0;
uint32_t lastTimeADC = 0;
uint32_t isOneWire = 0;
uint32_t isADC = 0;

void ICACHE_RAM_ATTR btnIsr() {
  btnStatus = 1;
}

void initGPIO() {
  for (uint8_t i = 0; i < ports_len; i++) {
    if (ports[i].mode == GPIO_MODE_ONEWIRE) {
      if (!isOneWire) {
        if (ds) delete ds;
        ds = new OneWire(ports[i].gpio);
        isOneWire = 1;
      }
      continue;
    }
    if (ports[i].mode == GPIO_MODE_PWM) {
      pinMode(ports[i].gpio, OUTPUT);
      analogWrite(ports[i].gpio, ports[i].value);
    } else if (ports[i].mode == GPIO_MODE_ADC) {
      pinMode(ports[i].gpio, INPUT);
      ports[i].value = analogRead(ports[i].gpio);
      isADC = 1;
    } else {
      pinMode(ports[i].gpio, ports[i].mode);
      if (ports[i].mode == OUTPUT || ports[i].mode == OUTPUT_OPEN_DRAIN) digitalWrite(ports[i].gpio, ports[i].value);

      ports[i].value = digitalRead(ports[i].gpio);
      ports[i].valueOld = ports[i].value;
      if ((ports[i].mode == INPUT || ports[i].mode == INPUT_PULLUP) && ports[i].interrupt) {
        attachInterrupt(digitalPinToInterrupt(ports[i].gpio), btnIsr, ports[i].interrupt);
      }
    }
  }
}

void getAll() {
  for (int i = 0; i < ports_len; i++) {
    if (ports[i].mode == GPIO_MODE_PWM || ports[i].mode == GPIO_MODE_ONEWIRE || ports[i].mode == GPIO_MODE_DAC) {
    } else if (ports[i].mode == GPIO_MODE_ADC) {
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
  if (port.mode == GPIO_MODE_PWM || port.mode == GPIO_MODE_ONEWIRE) {
  } else if (port.mode == GPIO_MODE_ADC) {
    port.value = analogRead(port.gpio);
  } else if (port.mode == GPIO_MODE_DAC) {
#if defined(ARDUINO_ESP32C3_DEV)
#elif defined(ESP32)
    dacWrite(port.gpio, port.value);
#endif
  } else {
    port.value = digitalRead(port.gpio);
  }
  updatePort();
}

// void emitButtonEvent(const char* format, ...) {
//   char buffer[32];
//   va_list args;
//   va_start(args, format);
//   vsnprintf(buffer, sizeof(buffer), format, args);
//   va_end(args);
//   emitEvent(buffer);
// }

void emitButtonEvent(Port* port, uint8_t type) {
  scriptRunner.emitEvent("btn", 4, port->gpio, port->value, type, port->count);
}

void loopInterrupt(uint32_t now) {
  for (int i = 0; i < ports_len; i++) {
    if (!(ports[i].mode == INPUT || ports[i].mode == INPUT_PULLUP) && !ports[i].interrupt) continue;
    uint32_t time = now - ports[i].pressStart;
    if (time > REPEAT_START_TIME) {
      if (ports[i].isPressed && ports[i].count == 1) {
        if (ports[i].isButton) {
          deviceGPIO(&ports[i], EVENT_REPEAT);
          scriptRunner.emitEvent("btn_repeat", 0, ports[i].gpio, ports[i].value);
        }
      } else {
        ports[i].count = 0;
      }
    }
    if (time > LONG_PRESS_TIME && time < REPEAT_START_TIME) {
      if (!ports[i].isPressed && ports[i].count == 1) {
        if (ports[i].isButton) {
          deviceGPIO(&ports[i], EVENT_LONG_PRESS);
          scriptRunner.emitEvent("btn_long", 2, ports[i].gpio, ports[i].value);
        }
        ports[i].count = 0;
      }
    }
    if (time > CLICK_WINDOW) {
      if (!ports[i].isPressed && ports[i].count) {
        if (ports[i].isButton) {
          deviceGPIO(&ports[i], EVENT_CLICK);
          scriptRunner.emitEvent("btn_click", 3, ports[i].gpio, ports[i].value, ports[i].count);
        }
        ports[i].count = 0;
      }
    }
  }
}

void checkInterrupt(uint32_t now) {
  for (int i = 0; i < ports_len; i++) {
    if (!(ports[i].mode == INPUT || ports[i].mode == INPUT_PULLUP) && !ports[i].interrupt) continue;

    port = ports[i];
    port.value = digitalRead(port.gpio);
    if (ports[i].interrupt == GPIO_INTERRUPT_CHANGE && ports[i].value != port.value) {
      ports[i].isPressed = ports[i].valueOld != port.value;
      if (ports[i].isPressed) {
        ports[i].count++;
        ports[i].pressStart = now;
      }
      ports[i].value = port.value;
      scriptRunner.emitEvent("btn", 2, ports[i].gpio, ports[i].value);
      deviceGPIO(&ports[i], EVENT_NONE);
    }
    wsSendAll((uint8_t*)&ports[i], sizeof(ports[i]));
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
  Serial.print("stateChangeProvider:");
  Serial.print(gpio);
  Serial.println(newValue);
  updatePort(gpio, newValue);
}

bool portProvider(uint8_t gpio, PortAction action, uint16_t& value) {
  Serial.print("portProvider:");
  Serial.print(gpio);
  Serial.println(value);
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
    checkInterrupt(now);
  }

  if (now - lastLoopInterrupt > REPEAT_INTERVAL) {
    lastLoopInterrupt = now;
    loopInterrupt(now);
  }

  if (now - lastTimeGPIO > 10000) {
    lastTimeGPIO = now;
    if (isOneWire) {
      findDallas();
    }
  }

  if (isADC && now - lastTimeADC > 5000) {
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
