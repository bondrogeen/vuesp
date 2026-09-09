#include "./device.h"

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE};
uint32_t lastTimeDevice = 0;
uint32_t lastTimeEnergy = 0;
// CircularBuffer<int8_t, 256> buffer(myBuffer);

const uint8_t reqVoltage[] = {0x01, 0x04, 0x00, 0x00, 0x00, 0x02, 0x71, 0xCB};      // Напряжение
const uint8_t reqCurrent[] = {0x01, 0x04, 0x00, 0x03, 0x00, 0x02, 0x81, 0xCB};      // Ток
const uint8_t reqActivePower[] = {0x01, 0x04, 0x00, 0x08, 0x00, 0x02, 0xF0, 0x09};  // Мощность
const uint8_t reqCosPhi[] = {0x01, 0x04, 0x00, 0x14, 0x00, 0x02, 0x31, 0xCF};       // Коэффициент мощности
const uint8_t reqFrequency[] = {0x01, 0x04, 0x00, 0x1A, 0x00, 0x02, 0x50, 0x0C};    // Частота (запятая на месте)

const uint8_t reqAdd[] = {0x01, 0x04, 0x00, 0x61, 0x00, 0x02, 0x20, 0x15};
const uint8_t reqBaud[] = {0x01, 0x03, 0x00, 0x62, 0x00, 0x02, 0x65, 0xD5};
const uint8_t reqFormat[] = {0x01, 0x04, 0x00, 0x63, 0x00, 0x02, 0x81, 0xD5};

const uint8_t reqActiveEnergy[] = {0x01, 0x04, 0x00, 0x1D, 0x00, 0x02, 0xE1, 0xCD};          // Общая энергия
const uint8_t reqPositiveActiveEnergy[] = {0x01, 0x04, 0x00, 0x27, 0x00, 0x02, 0xC1, 0xC0};  // Общая энергия
const uint8_t reqReverseActiveEnergy[] = {0x01, 0x04, 0x00, 0x31, 0x00, 0x02, 0x20, 0x04};   // Общая энергия
const uint8_t reqReactiveEnergy[] = {0x01, 0x04, 0x00, 0x3B, 0x00, 0x02, 0x00, 0x06};        // Общая энергия

uint8_t response[64];
uint8_t dataBuffer[4];

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
void deviceGPIO(Port* port, uint8_t type) {
  Serial.printf("gpio:%d, value:%d", port->gpio, port->value);
  if (type == EVENT_LONG_PRESS) {
    Serial.print(", type:long");
  } else if (type == EVENT_REPEAT) {
    Serial.print(", type:repeat");
  } else if (type == EVENT_CLICK) {
    Serial.printf(", type:click, count:%d", port->count);
  } else {
    // Serial.print(port->gpio);
  }
  Serial.println("");
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

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}

uint32_t parseFloat(const uint8_t* response, uint8_t offset) {
  uint8_t byte0 = response[offset];
  uint8_t byte1 = response[offset + 1];
  uint8_t byte2 = response[offset + 2];
  uint8_t byte3 = response[offset + 3];
  uint16_t highWord = (byte0 << 8) | byte1;
  uint16_t lowWord = (byte2 << 8) | byte3;
  uint32_t result = ((uint32_t)lowWord << 16) | highWord;
  return result;
}

uint32_t parseUint32(const uint8_t* response, uint8_t offset) {
  return ((uint32_t)response[offset] << 24) |
         ((uint32_t)response[offset + 1] << 16) |
         ((uint32_t)response[offset + 2] << 8) |
         ((uint32_t)response[offset + 3]);
}

uint8_t transmitData(uint8_t* buff, uint8_t isCheck) {
  uint8_t index = 0;
  uint32_t timeout = millis() + 100;

  while (Serial1.available() || millis() < timeout) {
    if (Serial1.available()) {
      buff[index] = Serial1.read();
      // if (buff[index] < 0x10) {
      //   Serial.print("0");
      // }
      // Serial.print(buff[index], HEX);
      // Serial.print(" ");
      index++;
    }
  }
  Serial.println("");
  if (isCheck) {
    uint16_t crc = getCrc16(buff, index - 2);
    uint8_t bytes[2];
    uint16ToByte(crc, bytes);
    if (bytes[0] == buff[index - 2] && bytes[1] == buff[index - 1]) {
      return index;
    }
    return 0;
  }
  return index;
}

uint8_t f = 0;

void getData() {
  Serial1.write(reqVoltage, sizeof(reqVoltage));
  if (transmitData(response, true)) {
    device.voltage = parseFloat(response, 3);
  }

  Serial1.write(reqCurrent, sizeof(reqCurrent));
  if (transmitData(response, true)) {
    device.current = parseFloat(response, 3);
  }

  Serial1.write(reqActivePower, sizeof(reqActivePower));
  if (transmitData(response, true)) {
    device.power = parseFloat(response, 3);
  }

  // Serial1.write(reqCosPhi, sizeof(reqCosPhi));
  // if (transmitData(response, true)) {
  //   uint32_t raw = parseFloat(response, 3);
  //   // float voltage = raw / 10.0f;
  //   Serial.println(raw);
  // }

  Serial1.write(reqFrequency, sizeof(reqFrequency));
  if (transmitData(response, true)) {
    device.frequency = parseFloat(response, 3);
  }
}

void getDataEnergy() {
  Serial1.write(reqActiveEnergy, sizeof(reqActiveEnergy));
  if (transmitData(response, true)) {
    device.activeEnergy = parseUint32(response, 3);
  }

  Serial1.write(reqPositiveActiveEnergy, sizeof(reqPositiveActiveEnergy));
  if (transmitData(response, true)) {
    device.positiveActiveEnergy = parseUint32(response, 3);
  }

  Serial1.write(reqReverseActiveEnergy, sizeof(reqReverseActiveEnergy));
  if (transmitData(response, true)) {
    device.reverseActiveEnergy = parseUint32(response, 3);
  }

  Serial1.write(reqReactiveEnergy, sizeof(reqReactiveEnergy));
  if (transmitData(response, true)) {
    device.reactiveEnergy = parseUint32(response, 3);
  }

  Serial1.write(reqBaud, sizeof(reqBaud));
  if (transmitData(response, true)) {
    device.baud = parseUint32(response, 3);
    Serial.println(device.baud);
  }
}

void setupDevice() {
  Serial1.begin(9600, SERIAL_8N1, 6, 5);
  getData();
  getDataEnergy();
  scriptRunner.registerFunction("http", httpHandler);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 5000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();
  }

  if (now - lastTimeEnergy > 1000 * 60) {
    lastTimeEnergy = now;
    getDataEnergy();
    onSendDevice();
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  };

  if (tasks[KEY_DEVICE]) {
    if (device.command == COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}