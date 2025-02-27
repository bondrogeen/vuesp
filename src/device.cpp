#include "./include/device.h"

#include <OneWire.h>
#include <SoftwareSerial.h>
#include <Wire.h>

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

EspSoftwareSerial::UART ModbusSerial;

OneWire ds(GPIO_HT1);

UnixTime stamp(0);

Dallas ht1 = {KEY_DALLAS, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
DDS6619 sinotimer = {KEY_DDS6619, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
Modbus modbus = {KEY_MODBUS, 0, 255, 0};
ModbusSetting modbusSetting = {
    KEY_MODBUS_SETTING,
    EspSoftwareSerial::SWSERIAL_8N2,
    9600,
    0,
};

Device device = {
    KEY_DEVICE,
    0,
    0,
    255,
    0,
    "test"};

uint8_t task;
uint32_t lastTimeDevice = 0;
uint32_t lastTimeModbus = 0;

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
    if (info->len == sizeof(modbus)) {
      uint8_t *address = (uint8_t *)&modbus;
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
void onSendModbus() {
  send((uint8_t *)&modbus, sizeof(modbus), KEY_MODBUS);
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
  onSend();
}

void setModbusSetup() {
  ModbusSerial.begin(9600, EspSoftwareSerial::SWSERIAL_8N2, GPIO_485RX, GPIO_485TX);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
  getLoadDef(DEF_PATH_MODBUS, (uint8_t *)&modbusSetting, sizeof(modbusSetting));
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

// uint8_t readModbus(uint16_t *value, uint16_t address, uint8_t length) {
//   uint8_t result = node.readInputRegisters(address, length);
//   Serial.println(result);
//   if (result == node.ku8MBSuccess) {
//     *value = node.getResponseBuffer(0);
//   }
//   return result;
// }

// void readHoldingRegistersDemo() {
//   readModbus(&ec.voltage, 0x00, 1);
//   readModbus(&ec.current, 0x03, 1);
//   readModbus(&ec.power, 0x08, 1);
//   readModbus(&ec.cos, 0x14, 1);
//   readModbus(&ec.frequency, 0x1A, 1);
// }

// byte message[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x01, 0x31, 0xca};
// ModbusSerial.print(0x01 04 00 00 00 01 31 ca);
// 01 04 00 00 00 01 31 ca
// 01 04 02 08 8a 3f 57
// 01 04 00 03 00 01 c1 ca
// 01 04 02 00 1c b8 f9
// 01 04 00 08 00 01 b0 08
// 01 04 02 00 37 f8 e6
// 01 04 00 14 00 01 71

void transmitData() {
  ModbusSerial.write(modbus.data, modbus.size);
  uint8_t bufferIndex = 0;
  u_int8_t status = 1;
  lastTimeModbus = millis();

  while (status) {
    if (ModbusSerial.available() > 0) {
      modbus.data[bufferIndex] = ModbusSerial.read();
      bufferIndex++;
      modbus.size = bufferIndex;
      if (!ModbusSerial.available() && bufferIndex) {
        status = 0;
      }
    }
    if (millis() - lastTimeModbus > 300) {
      status = 0;
    }
  }
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
      transmitData();
      onSendModbus();
    }
  }

  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;
    Serial.println(device.now);
    if (device.command == 1) {
      Serial.println("device");
      Serial.println(device.now);
      // writeFile(DEF_PATH_CONFIG, (uint8_t *)&device, sizeof(device));
    }

    device.command = 0;
    onSend();
  };
}
