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
uint32_t lastLoopInput = 0;
uint32_t deviceInputOld = 0;

struct PortDevice {
  uint8_t isPressed;
  uint8_t count;
  uint8_t value;
  uint8_t valueOld;
  uint32_t pressStart;
};

PortDevice portsDevice[PORTS_LEN] = {
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0},
};

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

void loopInput(uint32_t now) {
  for (int i = 0; i < PORTS_LEN; i++) {
    uint32_t time = now - portsDevice[i].pressStart;
    // Serial.println(time);
    // if (time > REPEAT_START_TIME) {
    //   if (portsDevice[i].isPressed && portsDevice[i].count == 1) {
    //     scriptRunner.emitEvent("port_r", 2, i, portsDevice[i].value);
    //   } else {
    //     portsDevice[i].count = 0;
    //   }
    // }
    // if (time > LONG_PRESS_TIME && time < REPEAT_START_TIME) {
    //   if (!portsDevice[i].isPressed && portsDevice[i].count == 1) {
    //     scriptRunner.emitEvent("port_l", 2, i, portsDevice[i].value);
    //     portsDevice[i].count = 0;
    //   }
    // }
    if (time > 400) {
      if (!portsDevice[i].isPressed && portsDevice[i].count) {
        Serial.println(time);
        Serial.println(portsDevice[i].count);
        scriptRunner.emitEvent("port_c", 3, i, portsDevice[i].value, portsDevice[i].count);
        portsDevice[i].count = 0;
      }
    }
  }
}

void findPort() {
  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    uint8_t valueNew = (device.input & (1 << i)) != 0;
    if (portsDevice[i].value != valueNew) {
      Serial.print(i);
      Serial.print(": ");
      Serial.println(valueNew);
      portsDevice[i].isPressed = portsDevice[i].valueOld != valueNew;
      if (portsDevice[i].isPressed) {
        Serial.print("isPressed");
        portsDevice[i].count++;
        Serial.print(": ");
        Serial.println(portsDevice[i].count);

        // Serial.println(portsDevice[i].valueOld);
        // Serial.println(valueNew);
      }
      portsDevice[i].pressStart = millis();
      portsDevice[i].value = valueNew;
      // scriptRunner.emitEvent("port", 2, i, portsDevice[i].value);
    }
  }
}

// only port.interrupt == GPIO_INTERRUPT_CHANGE
void deviceGPIO(Port* port, uint8_t type) {
  if (port->gpio == 13 && port->value == 0) {
    getInput();
    delay(50);
    if(digitalRead(13) == 0) {
      getInput();
    }
    // findPort();
    // onSendDevice();
  }

  Serial.printf("gpio:%d, value:%d", port->gpio, port->value);
  
  Serial.println(digitalRead(13));
  // if (type == EVENT_LONG_PRESS) {
  //   Serial.print(", type:long");
  // } else if (type == EVENT_REPEAT) {
  //   Serial.print(", type:repeat");
  // } else if (type == EVENT_CLICK) {
  //   Serial.printf(", type:click, count:%d", port->count);
  // } else {
  //   // Serial.print(port->gpio);
  // }
  Serial.println("");
}

static char displayBuffer[64] = "5";

bool inputHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount != 1) return false;
  const uint32_t pin = params[0].uintVal;
  if (pin >= 1 && pin < 7) {
    result.type = VAL_UINT;
    result.intVal = bitRead(device.input, pin - 1);
    return true;
  }
  return false;
}

bool outputHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 1) return false;
  const uint32_t pin = params[0].uintVal;
  const uint32_t value = params[1].uintVal;
  Serial.println(pin);
  Serial.println(value);
  if (pin >= 1 && pin < 7) {
    if (paramCount == 2) {
      if (value) {
        bitSet(device.output, pin - 1);
      } else {
        bitClear(device.output, pin - 1);
      }
      setOutput();
    }
    result.type = VAL_UINT;
    result.intVal = bitRead(device.output, pin - 1);
    return true;
  }
  return false;
}

// void sendNotification(const char* text) {
//   message.type = MESSAGE_TYPE_NOTIFICATION;
//   memset(message.text, 0, sizeof(message.text));
//   strcpy(message.text, text);
//   wsSendAll((uint8_t*)&message, sizeof(message));
// }

void setupDevice() {
  scriptRunner.registerFunction("input", inputHandler);
  scriptRunner.registerFunction("output", outputHandler);

  Wire.begin(GPIO_SDA, GPIO_SCL);
  Wire.setClock(400000);
  // setModbusSetup();
  setOutput();
  getDate();
  getOutput();
  getGPIO();
  for (uint8_t i = 0; i < PORTS_LEN; i++) {
    uint8_t valueNew = (device.input & (1 << i)) != 0;
    portsDevice[i].value = valueNew;
    portsDevice[i].valueOld = valueNew;
    portsDevice[i].count = 0;
    portsDevice[i].isPressed = 0;
  }
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
  // getLoadDef(DEF_PATH_MODBUS, (uint8_t *)&modbusSetting, sizeof(modbusSetting));
}

void loopDevice(uint32_t now) {
  if (now - lastLoopInput > REPEAT_INTERVAL) {
    lastLoopInput = now;
    loopInput(now);
  }

  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();
    // sendNotification("Test");
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
    // if (device.command == COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));

    device.command = 0;
    onSendDevice();
  };
}

// // внешняя функция получения текущего значения и управление состоянием шим
// bool portProvider(uint8_t gpio, uint8_t action, uint16_t& value) {
//   switch (action) {
//     case PORT_READ:
//       return getValue(gpio, value);
//     case PORT_WRITE:
//       setValue(gpio, value);
//       return true;
//   }
//   return false;
// }

// // внешняя функция которая обновляет значение перемененой (срабатывает только при старте (fabe.start(...)) и завершении )
// void stateChangeProvider(uint8_t gpio, uint16_t value) {
//   updatePort(gpio, value);
// }

// void setup() {
//   fabe.init(5); // 5 независимых каналов
//   fabe.setDataProvider(dataProvider);
//   fabe.setStateChangeProvider(stateChangeProvider);

//   fabe.start(13, 255, 2000); // 13-gpio, 255 value, 2000 = 2сек время за которое поменяется от текущего значения до 255
//   fabe.start(13, 0, 1000); // 13-gpio, 0 value, 1000 = 1сек время за которое поменяется от текущего значения до 0
// }

// void loop() {
//   fabe.loop();
// }