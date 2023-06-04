#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

uint32_t lastTimeDevice = 0;

Device device = {KEY_DEVICE, 0, 0, 0, ""};

void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len) {
    if (info->opcode == WS_BINARY) {
      uint8_t task = data[0];
      if (task == KEY_DEVICE && info->len == sizeof(device)) {
        memcpy(&device, data, sizeof(device));
        tasks[task] = true;
      }
    }
  }
}

void onSend(uint8_t direction) {
  device.example1 += 1;
  device.direction = direction;
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
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
