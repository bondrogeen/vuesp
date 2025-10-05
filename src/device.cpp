#include "./include/device.h"

#include <OneWire.h>
#include <Wire.h>

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/modbus.h"
#include "./include/tasks.h"

OneWire ds(GPIO_HT1);

UnixTime stamp(0);

Dallas ht1 = {KEY_DALLAS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
DDS6619 sinotimer = {KEY_DDS6619, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

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

uint32_t lastTimeDevice = 0;

void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
  if (task == KEY_MODBUS && info->len == sizeof(modbus)) {
    memcpy(&modbus, data, sizeof(modbus));
  }
}

void onSend() {
  sendAll((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}
void onSendTemp() {
  sendAll((uint8_t *)&ht1, sizeof(ht1), KEY_DALLAS);
}
void onSendModbus() {
  sendAll((uint8_t *)&modbus, sizeof(modbus), KEY_MODBUS);
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

void setOutput() {
  Wire.beginTransmission(ADDRESS_OUTPUT);
  Wire.write(device.output);
  Wire.endTransmission();
}

void setDAC() {
#if defined(ESP32)
  dacWrite(DAC1, device.dac1);
  dacWrite(DAC2, device.dac2);
#endif
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
  uint32_t unixTime = 0;
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
  unixTime = stamp.getUnix();
  device.now = unixTime;
  return unixTime;
}

void getGPIO() {
  getInput();
  onSend();
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  // getLoadDef(DEF_PATH_MODBUS, (uint8_t *)&modbusSetting, sizeof(modbusSetting));
}

void setupDevice() {
  Wire.begin(GPIO_SDA, GPIO_SCL);

  setModbusSetup();
  setOutput();
  setDAC();
  getADC();
  getDate();
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

void getData() {
  getADC();
  getInput();
  getOutput();
  getDate();
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSend();
  }

  if (tasks[KEY_MODBUS]) {
    tasks[KEY_MODBUS] = 0;
    if (modbus.command == 1) {
      uint8_t len = transmitData(modbus.data, modbus.size, 0);
      if (len) {
        modbus.size = len;
        onSendModbus();
      }
    }
  }

  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;

    if (device.command == 1) {
      setDate(device.now);
    } else if (device.command == 2) {
      setOutput();
    } else if (device.command == 3) {
      setDAC();
    } else if (device.command == 4) {
      device.command = 0;
      writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    } else {
      getData();
      findDallas();
    }

    device.command = 0;
    onSend();
  };
}
