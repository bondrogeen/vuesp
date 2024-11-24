#include "./include/device.h"

#include <Wire.h>

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
    0,
    0,
    0,
    0,
    0,
};

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

void getGpio() {
  getInput();
  onSend();
}

void getInput() {
  Wire.requestFrom(ADDRESS_INPUT, 1);
  while (Wire.available()) {
    device.input = Wire.read();
  }
}

void getOutput() {
  Wire.requestFrom(ADDRESS_OUTPUT, 1);
  while (Wire.available()) {
    device.output = Wire.read();
  }
}

void getADC() {
  device.adc1 = analogRead(GPIO_ADC1);
  device.adc2 = analogRead(GPIO_ADC2);
  device.adc3 = analogRead(GPIO_ADC3);
  device.adc4 = analogRead(GPIO_ADC4);
}

void setupDevice() {
  Wire.begin(GPIO_SDA, GPIO_SCL);

  Wire.beginTransmission(ADDRESS_OUTPUT);
  Wire.write(0);
  Wire.endTransmission();

  getADC();
  getGpio();
}

void scan() {
  byte error, address;
  int nDevices;
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();
    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16) Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16) Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

// I2C device found at address 0x22  !
// I2C device found at address 0x24  !
// I2C device found at address 0x68  !

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;

    getInput();
    getADC();
    onSend();
  }
  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;

    Wire.beginTransmission(ADDRESS_OUTPUT);
    Wire.write(device.output);
    Wire.endTransmission();
    onSend();
  };
}
