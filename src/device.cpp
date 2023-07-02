#include "./include/device.h"

#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(256, 14, NEO_GRB + NEO_KHZ800);

uint32_t lastTimeDevice = 0;

Device device = {KEY_DEVICE, 42, 57, {0xffffffff, 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0x000000ff, 0x00000000, 0x000000ff, 0x0000ff00, 0xffff0000, 5}};

uint8_t task;

void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;

  if (info->opcode == WS_BINARY) {
    if (info->index == 0) {
      task = data[0];
    }
    if (info->len == sizeof(device)) {
      Serial.println("fgghfgfgfg");
      Serial.println(len);
      Serial.println(info->len);
      Serial.println(info->index);
      uint8_t *address = (uint8_t*)&device;
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

void led() {
  // Serial.println("test");
  pixels.clear();

  for (int i = 0; i < NUMPIXELS; i++) {
    pixels.setPixelColor(i, device.buffer[i]);
    // Serial.println(device.buffer[i]);
  }
  pixels.show();
}

void setupDevice() {
  pixels.begin();
  pixels.setBrightness(5);

  led();
  // Serial.println("test");
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;

    send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
  }
  if (tasks[KEY_DEVICE]) {
    // onSend(device.direction);
    // Serial.println("test");
    led();
    tasks[KEY_DEVICE] = 0;
  };
}
