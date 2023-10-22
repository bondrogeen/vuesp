#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Device device = {KEY_DEVICE, 0, 0, 0, ""};

uint8_t task;
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

void onSend(uint8_t direction) {
  device.example1 += 1;
  device.direction = direction;
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}

void eventGPIO(uint8_t port, uint8_t value) {
  Serial.print(port);
  Serial.print(" : ");
  Serial.println(value);
}

void setupDevice() {
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    strcpy(device.buffer, "timeout: 10s");
    onSend(1);
  }
  if (tasks[KEY_DEVICE]) {
    onSend(device.direction);
    tasks[KEY_DEVICE] = 0;
  };
}
