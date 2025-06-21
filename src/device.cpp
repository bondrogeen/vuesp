#include "./include/device.h"

#include <OneWire.h>

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

OneWire ds(GPIO_18B20);

uint8_t address[8];

Device device = {
    KEY_DEVICE,
    0,
    255,
    0,
    0,
    0,
    0,
    0,
    0,
};

uint8_t direction = 0;
uint8_t start;
uint8_t task;
uint32_t lastTimeDeviceLight = 0;
uint32_t lastTimeDevice = 0;

void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;

  if (info->opcode == WS_BINARY) {
    if (info->index == 0) {
      task = data[0];
    }
    if (info->len == sizeof(device)) {
      uint8_t *address = (uint8_t *)&device;
      for (size_t i = 0; i < len; i++) {
        *(address + i + info->index) = *(data + i);
      }
    }
    if ((info->index + len) == info->len) {
      tasks[task] = true;
    }
  }
}

void onSend() {
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}

void deviceGPIO() {
  device.sensor1 = digitalRead(GPIO_SENSOR1);
  device.sensor2 = digitalRead(GPIO_SENSOR2);
  if (device.sensor1) {
    start = 1;
    direction = !direction;
  } else {
    if (start == 5) {
      start = 2;
    }
    if (start == 6) {
      start = 255;
    }
  }
  onSend();
}

void getDef() {
  uint8_t isOk = readFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  if (!isOk) {
    writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  }
}

float getTemperature(uint8_t *address1) {
  uint16_t temp;
  ds.reset();
  ds.select(address1);
  ds.write(0xBE);
  temp = (ds.read() | ds.read() << 8);

  ds.reset();
  ds.select(address1);
  ds.write(0x44, 1);
  return (float)temp / 16.0;
}

void findDallas() {
  while (ds.search(address) == 1) {
    device.temp = getTemperature(address);
  }
}

void getGPIO() {
  onSend();
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
}

void setupDevice() {
  analogWriteFreq(200);
  analogWrite(GPIO_PWM, device.state ? device.light : 255);
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    findDallas();
    onSend();
  }

  if (start == 1) {
    start = 5;
    lastTimeDeviceLight = now;
  }

  if ((start == 5) && now - lastTimeDeviceLight > 500) {
    start = 6;
    lastTimeDeviceLight = now;
  }
  if (start == 6 && now - lastTimeDeviceLight > 10) {
    lastTimeDeviceLight = now;
    if (direction && device.light < 255) {
      device.light++;
    }
    if (!direction && device.light > 0) {
      device.light--;
    }
    analogWrite(GPIO_PWM, device.light);
  }

  if (start == 2) {
    device.state = !device.state;
    analogWrite(GPIO_PWM, device.state ? device.light : 255);
    start = 255;
  }

  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;
    if (device.command == 1) {
      writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }
    if (device.command == 2) {
      analogWrite(GPIO_PWM, device.state ? device.light : 255);
    }

    device.command = 0;
    onSend();
  };
}
