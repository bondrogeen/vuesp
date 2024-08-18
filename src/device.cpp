#include "./include/device.h"

#include <Wire.h>

#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Device device = {KEY_DEVICE, 0, 0, 0, 0, 255, 255, 0, 0, 0, 0, 0};

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
      Serial.println(device.portOut1);
      Serial.println(device.portOut2);
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
  Wire.begin(GPIO_SDA, GPIO_SCL);
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 2000) {
    lastTimeDevice = now;

    Wire.requestFrom(0x21, 1);
    while (Wire.available()) {
      device.portIn1 = Wire.read();
    }
    Wire.requestFrom(0x22, 1);
    while (Wire.available()) {
      device.portIn2 = Wire.read();
    }
    Serial.println(device.portIn1, BIN);

    uint16_t adc1 = analogRead(GPIO_ADC1);
    Serial.println(adc1);
    uint16_t adc2 = analogRead(GPIO_ADC2);
    Serial.println(adc2);
    uint16_t adc3 = analogRead(GPIO_ADC3);
    Serial.println(adc3);
    uint16_t adc4 = analogRead(GPIO_ADC4);
    Serial.println(adc4);

    onSend(1);
  }
  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;

    Serial.println(device.portOut1);
    Serial.println(device.portOut2);

    Wire.beginTransmission(0x24);
    Wire.write(device.portOut1);
    Wire.endTransmission();
    Wire.beginTransmission(0x25);
    Wire.write(device.portOut2);
    Wire.endTransmission();
    // onSend(device.direction);
  };
}
