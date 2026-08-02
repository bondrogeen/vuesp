#include "./device.h"

#include <Wire.h>

UnixTime stamp(0);

// DDS6619 sinotimer = {KEY_DDS6619, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

Device device = {
    KEY_DEVICE,
    0,
    0,
    255,
};

uint32_t lastTimeDevice = 0;

Buffer myBuffer = {KEY_BUFFER};

// CircularBuffer<int8_t, 256> buffer(myBuffer);

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
  // if (task == KEY_MODBUS && info->len == sizeof(modbus)) {
  //   memcpy(&modbus, data, sizeof(modbus));
  // }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}
// void onSendModbus() {
//   wsSendAll((uint8_t*)&modbus, sizeof(modbus));
// }

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
  onSendDevice();
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

void getData() {
  getInput();
  getOutput();
  getDate();
}

// only port.interrupt == GPIO_INTERRUPT_CHANGE
void deviceGPIO(Port* port) {
  if (port->gpio == 13) {
    getGPIO();
  }
  // Serial.print(port->gpio);
  // Serial.println(port->value);
}

static char displayBuffer[64] = "5";

bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  Serial.print(id);
  Serial.print(write ? "WRITE" : "READ");
  Serial.print(" kind=");
  Serial.println(kind);

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        char buf[65];
        uint8_t len = value.stringVal.len;
        if (len > 64) len = 64;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
        strcpy(displayBuffer, buf);
        Serial.print("DISPLAY: ");
        Serial.println(buf);
      } else if (kind == KIND_INT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%d", value.intVal);
        Serial.print("DISPLAY: ");
        Serial.println(value.intVal);
      } else if (kind == KIND_UINT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%u", value.uintVal);
        Serial.print("DISPLAY: ");
        Serial.println(value.uintVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%.2f", value.floatVal);
        Serial.print("DISPLAY: ");
        Serial.println(value.floatVal);
      }
      return true;
    } else {
      if (kind == KIND_STRING) {
        value.stringVal.data = (uint8_t*)displayBuffer;
        value.stringVal.len = strlen(displayBuffer);
      } else if (kind == KIND_INT) {
        value.intVal = atoi(displayBuffer);
      } else if (kind == KIND_UINT) {
        value.uintVal = (uint32_t)atoi(displayBuffer);
      } else if (kind == KIND_FLOAT) {
        value.floatVal = atof(displayBuffer);
      }
      return true;
    }
  }
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;

  const char* method = params[0].stringVal.data;
  const char* url = params[1].stringVal.data;
  const int32_t count = params[2].intVal;
  const uint32_t num = params[3].uintVal;

  const uint8_t* data = params[4].arrayVal.data;
  uint8_t len = params[4].arrayVal.len;

  for (uint8_t i = 0; i < len; i++) {
    Serial.print(data[i]);
  }

  Serial.println("method");
  Serial.println(method);
  Serial.println(url);
  Serial.println(count);
  Serial.println(num);

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);

  Wire.begin(GPIO_SDA, GPIO_SCL);

  // setModbusSetup();
  setOutput();
  getDate();
  getOutput();
  getGPIO();
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
  // getLoadDef(DEF_PATH_MODBUS, (uint8_t *)&modbusSetting, sizeof(modbusSetting));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();
  }

  if (tasks[KEY_DEVICE]) {
    tasks[KEY_DEVICE] = 0;

    if (device.command == 1) {
      setDate(device.now);
    } else if (device.command == 2) {
      setOutput();
    } else if (device.command == 4) {
      device.command = 0;
      writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    } else {
      getData();
    }

    device.command = 0;
    onSendDevice();
  };
}
