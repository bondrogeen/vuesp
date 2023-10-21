
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/led.h"
#include "./include/tasks.h"

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

Device device = {KEY_DEVICE, 42, 57, {0xffffffff, 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0x000000ff, 0x00000000, 0x000000ff, 0x0000ff00, 0xffff0000, 5}};
Sensors sensors = {KEY_SENSORS};
Led ledSettings = {KEY_LED, 60, 0, 20};

uint8_t task;
uint32_t effTimer;
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
  Serial.println(value);
  if (port == 13 && value) {
    ledSettings.ledEffect++;
  }
}

void effectsTick(uint32_t now) {
  if (now - effTimer >= 60) {
    effTimer = now;

    switch (ledSettings.ledEffect) {
      case 1:
        ledEffectLighters();
        break;
      case 2:
        ledEffectFire();
        break;
      default:
        ledSettings.ledEffect = 0;
        ledClear();
    }

    ledShow();
  }
}

void setupDevice() {
  Wire.begin(0, 2);
  setupLed();
  aht.begin();
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
}

void loopDevice(uint32_t now) {
  effectsTick(now);

  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    // i2cSscan();
    sensors_event_t humidity, temp;
    aht.getEvent(&humidity, &temp);
    sensors.ahtTemperature = temp.temperature;
    sensors.ahtHumidity = humidity.relative_humidity;
    sensors.bmpTemperature = bmp.readTemperature();
    sensors.bmpPressure = bmp.readPressure();
    sensors.bmpAltitude = bmp.readAltitude(1013.25);
    send((uint8_t *)&sensors, sizeof(sensors), KEY_SENSORS);
  }
  if (tasks[KEY_DEVICE]) {
    // led();
    tasks[KEY_DEVICE] = 0;
  };
  if (tasks[KEY_LED]) {
    // led();
    tasks[KEY_LED] = 0;
  };
}
