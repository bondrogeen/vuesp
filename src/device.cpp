#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>
#include <Wire.h>

#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/led.h"
#include "./include/tasks.h"

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

Device device = {KEY_DEVICE, 42, 57, {0xffffffff, 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0x000000ff, 0x00000000, 0x000000ff, 0x0000ff00, 0xffff0000, 5}};

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

void effectsTick(uint32_t now) {
  if (now - effTimer >= 60) {
    effTimer = now;

    // ledEffectLighters();
    ledEffectFire();
    ledShow();
  }
}

void i2cSscan() {
  byte error, address;
  int nDevices;

  Serial.println("Scanning...");

  nDevices = 0;
  for (address = 1; address < 127; address++) {
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknown error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done\n");
}

void setupDevice() {
  Wire.begin(D3, D4);
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
    aht.getEvent(&humidity, &temp);  // populate temp and humidity objects with fresh data
    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degrees C");
    Serial.print("Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println("% rH");

    Serial.print(F("Temperature = "));
    Serial.print(bmp.readTemperature());
    Serial.println(" *C");

    Serial.print(F("Pressure = "));
    Serial.print(bmp.readPressure());
    Serial.println(" Pa");

    Serial.print(F("Approx altitude = "));
    Serial.print(bmp.readAltitude(1013.25)); /* Adjusted to local forecast! */
    Serial.println(" m");

    Serial.println();

    // send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
  }
  if (tasks[KEY_DEVICE]) {
    // led();
    tasks[KEY_DEVICE] = 0;
  };
}
