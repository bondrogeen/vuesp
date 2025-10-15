#include "./include/device.h"

#include <Wire.h>

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"
#include "./libs/CircularBuffer.cpp"

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE, 0, 0, 100, 1566565655, "test"};
uint32_t lastTimeDevice = 0;

CircularBuffer<int8_t, 256> buffer(myBuffer);

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

// only port.interrupt == GPIO_INTERRUPT_CHANGE
void deviceGPIO(Port* port) {
  Serial.print(port->gpio);
  Serial.println(port->value);
}

void deviceGPIOInterrupt() {
}

void getADC() {
  device.analog = analogRead(A0);
}

void getData() {
  getADC();
}

void setupDevice() {
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();

    buffer.push(random(-55, 55));
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  };

  if (tasks[KEY_DEVICE]) {
    if (device.command == DEVICE_COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    if (device.command == DEVICE_COMMAND_TEXT) sendNotification();

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}
