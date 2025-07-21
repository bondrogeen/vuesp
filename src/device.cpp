#include "./include/device.h"

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Device device = {
    KEY_DEVICE,
    0,
    0,
    255,
    0,
    0,
    0,
    "test"};

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

void onSend() {
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}

void deviceGPIO() {
  uint8_t value = digitalRead(13);
  Serial.println(value);
}

void getGPIO() {
  onSend();
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
}

void getADC() {
  device.analog = analogRead(A0);
}

void setPWM() {
  analogWrite(13, device.pwm);
}
void getInput() {
  device.gpio12 = digitalRead(12);
  device.gpio14 = digitalRead(14);
}
void setOutput() {
  digitalWrite(14, device.gpio14);
}

void getData() {
  getADC();
}

void setupDevice() {
  pinMode(14, OUTPUT);
  digitalWrite(14, LOW);
  setOutput();
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSend();
  }

  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;
    if (device.command == 2) {
      setOutput();
    }
    if (device.command == 3) {
      setPWM();
    }
    if (device.command == 4) {
      writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }

    device.command = 0;
    onSend();
  };
}
