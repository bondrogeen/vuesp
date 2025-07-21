
#include "./include/device.h"

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/led.h"
#include "./include/tasks.h"

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

sensors_event_t humidity, temp;

Device device = {KEY_DEVICE, COMMAND_GET_ALL, SPEED, EFFECT_DRAW, BRIGHTNESS, 0, 0};
Draw draw = {KEY_DRAW, COMMAND_GET_ALL, SPEED, EFFECT_DRAW, {}};
Sensors sensors = {KEY_SENSORS};

uint8_t state;
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
    if (info->len == sizeof(draw)) {
      uint8_t *address = (uint8_t *)&draw;
      for (size_t i = 0; i < len; i++) {
        *(address + i + info->index) = *(data + i);
      }
    }
    if (info->len == sizeof(sensors)) {
      uint8_t *address = (uint8_t *)&sensors;
      for (size_t i = 0; i < len; i++) {
        *(address + i + info->index) = *(data + i);
      }
    }
    if ((info->index + len) == info->len) {
      tasks[task] = true;
      Serial.println("task");
      Serial.println(task);
    }
  }
}

void onSend() {
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}

void deviceGPIO() {
  uint8_t button = digitalRead(GPIO_BTN);
  uint8_t sensor = digitalRead(GPIO_HLK_LD101);
  device.sensor = sensor;
  device.button = button;
  if (button) {
    device.effect++;
  }
  onSend();
}

void effectsTick(uint32_t now) {
  if (now - effTimer >= device.speed) {
    effTimer = now;
    state = 1;

    switch (device.effect) {
      case EFFECT_LIGHTERS:
        ledEffectLighters();
        break;
      case EFFECT_FIRE:
        ledEffectFire();
        break;
      case EFFECT_RAINBOW:
        rainbow_fade();
        break;
      case EFFECT_TWINKLING_STARS:
        twinklingStars();
        break;
      case EFFECT_CHASE:
        theaterChase();
        break;
      default:
        device.effect = 0;
        ledClear();
    }

    ledShow();
  }
}

void getSensors() {

  // aht.getEvent(&humidity, &temp);

  sensors.status = aht.getEvent(&humidity, &temp);

  sensors.ahtTemperature = temp.temperature;
  sensors.ahtHumidity = humidity.relative_humidity;
  Serial.println(sensors.ahtTemperature);
  Serial.println(sensors.ahtHumidity);

  sensors.bmpTemperature = bmp.readTemperature();
  sensors.bmpPressure = bmp.readPressure();
  sensors.bmpAltitude = bmp.readAltitude(1013.25);
  send((uint8_t *)&sensors, sizeof(sensors), KEY_SENSORS);
}

void loopDevice(uint32_t now) {
  if (device.effect) {
    effectsTick(now);
  } else {
    if (state) {
      state = 0;
      ledClear();
      ledShow();
    }
  }

  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getSensors();
    onSend();
  }

  if (tasks[KEY_DEVICE]) {
    if (device.command == COMMAND_GET_ALL) {
      readFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }
    if (device.command == COMMAND_SET) {
      ledBrightness(device.brightness);
    }
    if (device.command == COMMAND_SAVE) {
      device.command = COMMAND_GET_ALL;
      writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }
    onSend();
    tasks[KEY_DEVICE] = 0;
  }

  if (tasks[KEY_SENSORS]) {
    getSensors();
    tasks[KEY_SENSORS] = 0;
  }
  if (tasks[KEY_DRAW]) {
    if (draw.command == COMMAND_DRAW) {
      led(draw);
    }
    send((uint8_t *)&draw, sizeof(draw), KEY_DRAW);
    tasks[KEY_DRAW] = 0;
  }
}

void setupDevice() {
  setupLed();
  ledBrightness(device.brightness);
  Wire.begin(GPIO_SDA, GPIO_SCL);
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
  aht.begin();
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
}
