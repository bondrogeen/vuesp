#include "./include/device.h"

#include <OneWire.h>
#include <Wire.h>

#include "./include/UnixTime.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

OneWire ds(GPIO_HT1);

UnixTime stamp(0);

Dallas ht1 = {KEY_DALLAS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Device device = {
    KEY_DEVICE,
    0,
    0,
    0,
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
void onSendTemp() {
  send((uint8_t *)&ht1, sizeof(ht1), KEY_DALLAS);
}

void getDef() {
  uint8_t isOk = readFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  if (!isOk) {
#if defined(ESP32)
    if (createDir(DEF_DIR_DEVICE)) {
      writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }
#else
    writeFile(DEF_PATH_CONFIG, (uint8_t *)device, sizeof(device));
#endif
  }
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

uint8_t bcdToDec(uint8_t val) {
  return ((val / 0x10) * 10) + (val % 0x10);
}

uint8_t decToBcd(uint8_t val) {
  return ((val / 10) * 0x10) + (val % 10);
}

void setDate(uint32_t unixTime) {
  stamp.getDateTime(unixTime);
  Wire.beginTransmission(ADDRESS_RTC);
  Wire.write(0);
  Wire.write(decToBcd(stamp.second));
  Wire.write(decToBcd(stamp.minute));
  Wire.write(decToBcd(stamp.hour));
  Wire.write(decToBcd(stamp.dayOfWeek));
  Wire.write(decToBcd(stamp.day));
  Wire.write(decToBcd(stamp.month));
  Wire.write(decToBcd(stamp.year - 2000));
  Wire.endTransmission(true);
}

uint32_t getDate() {
  Wire.beginTransmission(ADDRESS_RTC);
  Wire.write(0);
  Wire.endTransmission(true);

  Wire.requestFrom(ADDRESS_RTC, 7);
  uint8_t second = bcdToDec(Wire.read());
  uint8_t minute = bcdToDec(Wire.read());
  uint8_t hour = bcdToDec(Wire.read() & 0b111111);  // 24 hour time
  uint8_t weekDay = bcdToDec(Wire.read());          // 0-6 -> Sunday - Saturday
  uint8_t monthDay = bcdToDec(Wire.read());
  uint8_t month = bcdToDec(Wire.read());
  uint8_t year = bcdToDec(Wire.read());

  stamp.setDateTime(2000 + year, month, monthDay, hour, minute, second);
  return stamp.getUnix();
}

void getGPIO() {
  getInput();
  onSend();
  // getDef();
}

void setupDevice() {
  Wire.begin(GPIO_SDA, GPIO_SCL);
  getADC();
  device.now = getDate();
  getOutput();
  getGPIO();
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

float getTemperature(uint8_t *address1) {
  uint16_t temp;
  ds.reset();
  ds.select(address1);
  ds.write(0xBE);
  temp = (ds.read() | ds.read() << 8);

  ds.reset();
  ds.select(address1);
  ds.write(0x44, 1);
  return (float)temp / 16.0;
}

void findDallas() {
  while (ds.search(ht1.address) == 1) {
    ht1.temp = getTemperature(ht1.address);
    onSendTemp();
  }
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;

    getInput();
    getADC();
    onSend();
    findDallas();
  }
  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;

    if (device.command == 1) {
      setDate(device.now);
    }
    if (device.command == 2) {
      Wire.beginTransmission(ADDRESS_OUTPUT);
      Wire.write(device.output);
      Wire.endTransmission();
    }
    if (device.command == 3) {
      dacWrite(DAC1, device.dac1);
      dacWrite(DAC2, device.dac2);
    }
    device.now = getDate();
    device.command = 0;
    onSend();
  };
}
