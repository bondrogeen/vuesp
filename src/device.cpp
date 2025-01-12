#include "./include/device.h"

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Device device = {
    KEY_DEVICE,
    0,
    255,
    0,
    0,
};

uint8_t valueLight = 0;
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
  if (digitalRead(13)) {
    start = 1;
    direction = !direction;
  } else {
    if (start == 5) {
      start = 2;
    }
    if (start == 6) {
      start = 255;
    }
    onSend();
  }
  Serial.println(start);
}

void getDef() {
  uint8_t isOk = readFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  Serial.println("device");
  Serial.println(device.output);
  if (!isOk) {
    writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  }
}

void getGPIO() {
  onSend();
}

void setupFirstDevice() {
  getDef();
}

void setupDevice() {
  analogWrite(4, device.light);
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    // onSend();
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

    Serial.println(device.light);
    analogWrite(4, device.light);
  }

  if (start == 2) {
    valueLight = !valueLight;

    if (valueLight) {
      analogWrite(4, device.light);
    } else {
      analogWrite(4, 255);
    }
    Serial.println(now - lastTimeDeviceLight);
    start = 255;
  }

  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;
    if (device.command == 1) {
      writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }
    if (device.command == 2) {
      analogWrite(4, device.light);
    }

    device.command = 0;
    onSend();
  };
}
