#include "./modbus.h"

#include <Arduino.h>

#include "./modbus_master.h"
#include "./modbus_raw.h"
#include "./modbus_source.h"

#include "../config/ConfigStore.h"
#include "../device/device.h"
#include "../init/init.h"
#include "../tasks/tasks.h"
#include "../webserver/webserver.h"

#define MODBUS_FRAME_GAP_MS 5
#define MODBUS_MAX_FRAME 256
#define MODBUS_REG_COUNT (MODBUS_SLAVE_ADDR_START + MODBUS_SLAVE_REG_COUNT)
#define MODBUS_COIL_COUNT 0
#define MODBUS_DISCRETE_COUNT 0

#define MODBUS_EX_ILLEGAL_FUNCTION 1
#define MODBUS_EX_ILLEGAL_ADDRESS 2
#define MODBUS_EX_ILLEGAL_VALUE 3

Modbus modbus = {KEY_MODBUS, 0, MODBUS_DEFAULT_ROLE, MODBUS_DEFAULT_ADDRESS, MODBUS_DEFAULT_BAUD,
                 MODBUS_DATA_BITS, MODBUS_PARITY_NONE, 1, MODBUS_DEFAULT_TX_PIN, MODBUS_DEFAULT_RX_PIN,
                 MODBUS_DEFAULT_DE_PIN, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0}, 0};
ModbusProfile modbusProfile;
static ModbusProfile* incomingProfile = nullptr;

uint8_t* modbusProfileBuffer() {
  if (!incomingProfile) incomingProfile = new ModbusProfile;
  return (uint8_t*)incomingProfile;
}

static uint8_t rxFrame[MODBUS_MAX_FRAME];
static uint16_t rxLen = 0;
static uint32_t lastByte = 0;
static uint32_t lastStatus = 0;
static bool uartStarted = false;
static bool statusDirty = false;

uint16_t modbusCrc16(const uint8_t* data, size_t len) {
  uint16_t crc = 0xFFFF;
  for (size_t i = 0; i < len; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 1) ? (uint16_t)((crc >> 1) ^ 0xA001) : (uint16_t)(crc >> 1);
    }
  }
  return crc;
}

static bool frameCrcOk(const uint8_t* frame, size_t len) {
  if (len < 4) return false;
  const uint16_t crc = modbusCrc16(frame, len - 2);
  return frame[len - 2] == (uint8_t)(crc & 0xFF) && frame[len - 1] == (uint8_t)(crc >> 8);
}

void modbusAppendCrc(uint8_t* frame, size_t len) {
  const uint16_t crc = modbusCrc16(frame, len);
  frame[len] = (uint8_t)(crc & 0xFF);
  frame[len + 1] = (uint8_t)(crc >> 8);
}

void modbusRequestSend() { statusDirty = true; }

static uint16_t baudIndex() {
  if (modbus.baud == 4800) return 0;
  if (modbus.baud == 19200) return 2;
  return 1;
}

static int readCoil(uint16_t) { return -1; }
static int readDiscrete(uint16_t) { return -1; }
static bool writeCoil(uint16_t, bool) { return false; }

static const ModbusPoint* findSlavePoint(uint16_t address) {
  for (uint8_t i = 0; i < modbusProfile.pointCount; i++) {
    const ModbusPoint& point = modbusProfile.points[i];
    if (point.device != MODBUS_DEVICE_SLAVE) continue;
    const uint8_t width = modbusPointRegisters(point);
    if (address >= point.address && address < (uint32_t)point.address + width) return &point;
  }
  return nullptr;
}

static bool readRegister(uint16_t index, uint16_t& value) {
  if (index < MODBUS_SLAVE_ADDR_START) {
    switch (index) {
    case 0:
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
      value = 0;
      return true;
    case 8:
      value = infoFS.firmware[0];
      return true;
    case 9:
      value = infoFS.firmware[1];
      return true;
    case 10:
      value = infoFS.firmware[2];
      return true;
    case 11: {
      const uint32_t uptime = (uint32_t)infoFS.uptime;
      value = (uint16_t)(uptime & 0xFFFF);
      return true;
    }
    case 12: {
      const uint32_t uptime = (uint32_t)infoFS.uptime;
      value = (uint16_t)((uptime >> 16) & 0xFFFF);
      return true;
    }
    case 13:
      value = modbus.address;
      return true;
    case 14:
      value = baudIndex();
      return true;
    case 15:
      value = modbus.role;
      return true;
    case 16:
      value = modbus.lastError;
      return true;
    case 17:
      value = (uint16_t)(modbus.requests & 0xFFFF);
      return true;
    case 18:
      value = (uint16_t)(modbus.errors & 0xFFFF);
      return true;
    case 19:
      value = (uint16_t)(device.now & 0xFFFF);
      return true;
    case 20:
      value = (uint16_t)((device.now >> 16) & 0xFFFF);
      return true;
    case 21:
    case 22:
      value = 0;
      return true;
      default:
        value = 0;
        return true;
    }
  }

  const ModbusPoint* point = findSlavePoint(index);
  if (!point || point->var >= modbus.varCount) {
    value = 0;
    return true;
  }

  uint8_t encoded[MODBUS_ENCODE_MAX_REGS * 2] = {0};
  const ModbusVarValue& live = modbus.vars[point->var];
  modbusEncodeRegisters(*point, live.value, live.text, encoded);
  const uint8_t offset = (uint8_t)(index - point->address);
  value = (uint16_t)((encoded[offset * 2] << 8) | encoded[offset * 2 + 1]);
  return true;
}

static bool writeHolding(uint16_t index, uint16_t value) {
  if (index < MODBUS_SLAVE_ADDR_START) return false;

  const ModbusPoint* point = findSlavePoint(index);
  if (!point || !(point->flags & MODBUS_FLAG_WRITABLE)) return false;
  if (modbusPointRegisters(*point) != 1) return false;
  modbusSetVarValue(point->var, value, nullptr);
  return true;
}

static void loopLocalSources(uint32_t now) {
  static uint32_t lastLocal = 0;
  if ((uint32_t)(now - lastLocal) < 1000) return;
  lastLocal = now;

  for (uint8_t i = 0; i < modbusProfile.varCount; i++) {
    const ModbusVar& variable = modbusProfile.vars[i];
    if (!variable.source[0]) continue;

    double value = 0;
    uint8_t quality = MODBUS_Q_DISABLED;
    char text[16] = {0};
    modbusReadVarSource(i, value, text, sizeof(text), quality);
    modbusUpdateVarValue(i, value, text[0] ? text : nullptr, quality);
  }
}

bool modbusTransmit(const uint8_t* data, size_t len) {
  if (!uartStarted) return false;
  if (!data || len == 0 || len > MODBUS_MAX_FRAME) return false;
  if (modbus.dePin >= 0) digitalWrite((uint8_t)modbus.dePin, HIGH);
  Serial1.write(data, len);
  Serial1.flush();
  if (modbus.dePin >= 0) digitalWrite((uint8_t)modbus.dePin, LOW);
  return true;
}

static void sendFrame(uint8_t address, const uint8_t* data, size_t len) {
  if (address == 0) return;
  modbusTransmit(data, len);
}

static void sendException(uint8_t address, uint8_t function, uint8_t code) {
  modbus.errors++;
  modbus.lastError = (code == MODBUS_EX_ILLEGAL_FUNCTION)
                         ? MODBUS_ERR_FUNCTION
                         : (code == MODBUS_EX_ILLEGAL_ADDRESS ? MODBUS_ERR_RANGE : MODBUS_ERR_FRAME);

  if (address == 0) return;

  uint8_t out[5] = {address, (uint8_t)(function | 0x80), code, 0, 0};
  modbusAppendCrc(out, 3);
  sendFrame(address, out, sizeof(out));
}

static void processReadBits(uint8_t address, uint8_t function, const uint8_t* frame, size_t len, bool coils) {
  if (len != 8) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }

  const uint16_t start = (uint16_t)((frame[2] << 8) | frame[3]);
  const uint16_t count = (uint16_t)((frame[4] << 8) | frame[5]);
  const uint16_t total = coils ? MODBUS_COIL_COUNT : MODBUS_DISCRETE_COUNT;

  if (count < 1 || count > 2000) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if ((uint32_t)start + count > total) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }

  const uint8_t bytes = (uint8_t)((count + 7) / 8);
  uint8_t out[MODBUS_MAX_FRAME];
  out[0] = address;
  out[1] = function;
  out[2] = bytes;
  memset(out + 3, 0, bytes);

  for (uint16_t i = 0; i < count; i++) {
    const int value = coils ? readCoil((uint16_t)(start + i)) : readDiscrete((uint16_t)(start + i));
    if (value < 0) {
      sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
      return;
    }
    if (value) out[3 + (i / 8)] |= (uint8_t)(1u << (i % 8));
  }

  modbusAppendCrc(out, (size_t)3 + bytes);
  sendFrame(address, out, (size_t)3 + bytes + 2);
}

static void processReadRegisters(uint8_t address, uint8_t function, const uint8_t* frame, size_t len) {
  if (len != 8) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }

  const uint16_t start = (uint16_t)((frame[2] << 8) | frame[3]);
  const uint16_t count = (uint16_t)((frame[4] << 8) | frame[5]);

  if (count < 1 || count > 125) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if ((uint32_t)start + count > MODBUS_REG_COUNT) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }

  uint8_t out[MODBUS_MAX_FRAME];
  out[0] = address;
  out[1] = function;
  out[2] = (uint8_t)(count * 2);

  for (uint16_t i = 0; i < count; i++) {
    uint16_t value = 0;
    if (!readRegister((uint16_t)(start + i), value)) {
      sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
      return;
    }
    out[3 + i * 2] = (uint8_t)(value >> 8);
    out[4 + i * 2] = (uint8_t)(value & 0xFF);
  }

  modbusAppendCrc(out, (size_t)3 + count * 2);
  sendFrame(address, out, (size_t)3 + count * 2 + 2);
}

static void processWriteSingleCoil(uint8_t address, uint8_t function, const uint8_t* frame, size_t len) {
  if (len != 8) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }

  const uint16_t index = (uint16_t)((frame[2] << 8) | frame[3]);
  const uint16_t value = (uint16_t)((frame[4] << 8) | frame[5]);

  if (value != 0xFF00 && value != 0x0000) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if (!writeCoil(index, value == 0xFF00)) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }

  sendFrame(address, frame, len);
}

static void processWriteMultipleCoils(uint8_t address, uint8_t function, const uint8_t* frame, size_t len) {
  if (len < 9) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }

  const uint16_t start = (uint16_t)((frame[2] << 8) | frame[3]);
  const uint16_t count = (uint16_t)((frame[4] << 8) | frame[5]);
  const uint8_t bytes = frame[6];

  if (count < 1 || count > MODBUS_COIL_COUNT) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if (bytes != (uint8_t)((count + 7) / 8) || len != (size_t)9 + bytes) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if ((uint32_t)start + count > MODBUS_COIL_COUNT) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }

  for (uint16_t i = 0; i < count; i++) {
    const bool on = (frame[7 + (i / 8)] & (uint8_t)(1u << (i % 8))) != 0;
    if (!writeCoil((uint16_t)(start + i), on)) {
      sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
      return;
    }
  }

  uint8_t out[8] = {address, function, frame[2], frame[3], frame[4], frame[5], 0, 0};
  modbusAppendCrc(out, 6);
  sendFrame(address, out, sizeof(out));
}

static void processWriteSingleRegister(uint8_t address, uint8_t function, const uint8_t* frame, size_t len) {
  if (len != 8) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }

  const uint16_t index = (uint16_t)((frame[2] << 8) | frame[3]);
  const uint16_t value = (uint16_t)((frame[4] << 8) | frame[5]);

  if (index >= MODBUS_REG_COUNT) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }
  if (!writeHolding(index, value)) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }

  sendFrame(address, frame, len);
}

static void processWriteMultipleRegisters(uint8_t address, uint8_t function, const uint8_t* frame, size_t len) {
  if (len < 11) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }

  const uint16_t start = (uint16_t)((frame[2] << 8) | frame[3]);
  const uint16_t count = (uint16_t)((frame[4] << 8) | frame[5]);
  const uint8_t bytes = frame[6];

  if (count < 1 || count > 123) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if (bytes != (uint8_t)(count * 2) || len != (size_t)9 + bytes) {
    sendException(address, function, MODBUS_EX_ILLEGAL_VALUE);
    return;
  }
  if ((uint32_t)start + count > MODBUS_REG_COUNT) {
    sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
    return;
  }

  // Validate the whole range before writing: never apply half of a slave value.
  for (uint16_t i = 0; i < count; i++) {
    const ModbusPoint* point = findSlavePoint((uint16_t)(start + i));
    if (!point) continue;
    const bool full = start == point->address && count == modbusPointRegisters(*point);
    if (!full || !(point->flags & MODBUS_FLAG_WRITABLE)) {
      sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
      return;
    }
  }

  const ModbusPoint* slavePoint = findSlavePoint(start);
  if (slavePoint && start == slavePoint->address && count == modbusPointRegisters(*slavePoint)) {
    char text[16] = {0};
    const double value = modbusDecodeRegisters(*slavePoint, frame + 7, count, text, sizeof(text));
    modbusSetVarValue(slavePoint->var, value, text[0] ? text : nullptr);
    uint8_t out[8] = {address, function, frame[2], frame[3], frame[4], frame[5], 0, 0};
    modbusAppendCrc(out, 6);
    sendFrame(address, out, sizeof(out));
    return;
  }

  for (uint16_t i = 0; i < count; i++) {
    const uint16_t value = (uint16_t)((frame[7 + i * 2] << 8) | frame[8 + i * 2]);
    if (!writeHolding((uint16_t)(start + i), value)) {
      sendException(address, function, MODBUS_EX_ILLEGAL_ADDRESS);
      return;
    }
  }

  uint8_t out[8] = {address, function, frame[2], frame[3], frame[4], frame[5], 0, 0};
  modbusAppendCrc(out, 6);
  sendFrame(address, out, sizeof(out));
}

static void processFrame(const uint8_t* frame, size_t len) {
  if (len < 4 || len > MODBUS_MAX_FRAME) {
    modbus.errors++;
    modbus.lastError = MODBUS_ERR_FRAME;
    return;
  }

  if (!frameCrcOk(frame, len)) {
    modbus.errors++;
    modbus.lastError = MODBUS_ERR_CRC;
    return;
  }

  const uint8_t address = frame[0];
  if (address != modbus.address && address != 0) return;

  const uint8_t function = frame[1];
  modbus.requests++;
  modbus.lastAddress = address;
  modbus.lastFunction = function;
  modbus.lastError = MODBUS_ERR_NONE;

  if (address == 0 && function >= 0x01 && function <= 0x04) return;

  switch (function) {
    case 0x01:
      processReadBits(address, function, frame, len, true);
      return;
    case 0x02:
      processReadBits(address, function, frame, len, false);
      return;
    case 0x03:
    case 0x04:
      processReadRegisters(address, function, frame, len);
      return;
    case 0x05:
      processWriteSingleCoil(address, function, frame, len);
      return;
    case 0x06:
      processWriteSingleRegister(address, function, frame, len);
      return;
    case 0x10:
      processWriteMultipleRegisters(address, function, frame, len);
      return;
    case 0x0F:
      processWriteMultipleCoils(address, function, frame, len);
      return;
    default:
      sendException(address, function, MODBUS_EX_ILLEGAL_FUNCTION);
      return;
  }
}

static void sendModbus() {
  if (wsClientCount() > 0) wsSendAll((uint8_t*)&modbus, sizeof(modbus));
}

static void saveConfig() {
  modbus.key = KEY_MODBUS;
  const TConfigStatus status = configStore.write("modbus", &modbus, sizeof(modbus), CONFIG_MODBUS_VERSION);
#if VUESP_LOG_LEVEL >= 1
  if (status != TConfigStatus::Ok) {
    Serial.printf("[config] modbus write: %d\n", (int)status);
  }
#endif
}

static void stopUart() {
  if (uartStarted) {
    Serial1.end();
    uartStarted = false;
  }
  if (modbus.dePin >= 0) {
    pinMode((uint8_t)modbus.dePin, OUTPUT);
    digitalWrite((uint8_t)modbus.dePin, LOW);
  }
  rxLen = 0;
}

static void startUart() {
  stopUart();
#if defined(ARDUINO_ARCH_ESP32)
  if (modbus.dePin >= 0) {
    pinMode((uint8_t)modbus.dePin, OUTPUT);
    digitalWrite((uint8_t)modbus.dePin, LOW);
  }

  uint32_t config = SERIAL_8N1;
  if (modbus.parity == MODBUS_PARITY_EVEN) {
    config = (modbus.stopBits == 2) ? SERIAL_8E2 : SERIAL_8E1;
  } else if (modbus.parity == MODBUS_PARITY_ODD) {
    config = (modbus.stopBits == 2) ? SERIAL_8O2 : SERIAL_8O1;
  } else if (modbus.stopBits == 2) {
    config = SERIAL_8N2;
  }

  Serial1.begin(modbus.baud, config, modbus.rxPin, modbus.txPin);
  uartStarted = true;
#endif
}

static void applyConfig() {
  if (modbus.dataBits != MODBUS_DATA_BITS) modbus.dataBits = MODBUS_DATA_BITS;
  if (modbus.parity > MODBUS_PARITY_ODD) modbus.parity = MODBUS_PARITY_NONE;
  if (modbus.stopBits != 1 && modbus.stopBits != 2) modbus.stopBits = 1;
  if (modbus.txPin > 48) modbus.txPin = MODBUS_DEFAULT_TX_PIN;
  if (modbus.rxPin > 48) modbus.rxPin = MODBUS_DEFAULT_RX_PIN;
  if (modbus.dePin < -1 || modbus.dePin > 48) modbus.dePin = MODBUS_DEFAULT_DE_PIN;

  if (modbus.role == MODBUS_ROLE_SLAVE) {
    if (modbus.address < 1 || modbus.address > 247) modbus.address = MODBUS_DEFAULT_ADDRESS;
    if (modbus.baud != 4800 && modbus.baud != 9600 && modbus.baud != 19200) modbus.baud = MODBUS_DEFAULT_BAUD;
    startUart();
#if defined(ARDUINO_ARCH_ESP32)
    modbus.status = 1;
#else
    modbus.status = 0;
#endif
  } else if (modbus.role == MODBUS_ROLE_MASTER) {
    if (modbus.baud != 4800 && modbus.baud != 9600 && modbus.baud != 19200) modbus.baud = MODBUS_DEFAULT_BAUD;
    resetMaster();
    startUart();
#if defined(ARDUINO_ARCH_ESP32)
    modbus.status = 1;
#else
    modbus.status = 0;
#endif
  } else {
    stopUart();
    modbus.status = 0;
  }
}

static void handleCommand() {
  if (!tasks[KEY_MODBUS]) return;
  tasks[KEY_MODBUS] = 0;

  if (modbus.command == CMD_SAVE) {
    if (modbus.role > MODBUS_ROLE_MASTER) modbus.role = MODBUS_ROLE_OFF;
    applyConfig();
    saveConfig();
  } else if (modbus.command == CMD_SET) {
    if (modbus.role > MODBUS_ROLE_MASTER) modbus.role = MODBUS_ROLE_OFF;
    applyConfig();
  } else if (modbus.command == CMD_WRITE) {
    if (modbus.role == MODBUS_ROLE_MASTER) {
      // 1 = queued, 0 = point missing or not writable. Visible in MODBUS.
      modbus.ok = masterQueueWrite(modbus.point, modbus.value, modbus.text) ? 1 : 0;
    }
  }

  modbus.command = 0;
  sendModbus();
}

void setupFirstModbus() {
  if (!loadModbusProfile(modbusProfile)) {
    defaultModbusProfile(modbusProfile);
  }

  const TConfigStatus status = configStore.read("modbus", &modbus, sizeof(modbus), CONFIG_MODBUS_VERSION);
  if (status == TConfigStatus::Ok) {
    modbus.key = KEY_MODBUS;
    modbus.command = 0;
    modbus.status = 0;
    modbus.lastFunction = 0;
    modbus.lastAddress = 0;
    modbus.lastError = MODBUS_ERR_NONE;
    modbus.requests = 0;
    modbus.errors = 0;
  } else {
    modbus = {KEY_MODBUS, 0, MODBUS_DEFAULT_ROLE, MODBUS_DEFAULT_ADDRESS, MODBUS_DEFAULT_BAUD,
              MODBUS_DATA_BITS, MODBUS_PARITY_NONE, 1, MODBUS_DEFAULT_TX_PIN, MODBUS_DEFAULT_RX_PIN,
              MODBUS_DEFAULT_DE_PIN, 0, 0, 0, 0, 0, 0, 0, {0, 0, 0}, 0};
  }

  if (modbus.role > MODBUS_ROLE_MASTER) modbus.role = MODBUS_ROLE_OFF;
}

void setupModbus() {
  modbus.key = KEY_MODBUS;
  modbus.command = 0;
  setupRaw();
  registerMasterFunctions();
  applyConfig();
  resetMaster();
}

void onWsEventModbus(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || task != KEY_MODBUS) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->len != sizeof(modbus)) return;

  Modbus incoming;
  memcpy(&incoming, data, sizeof(incoming));
  incoming.key = KEY_MODBUS;

  // GET is a request with a full-size zeroed payload. Write carries only the
  // write fields; everything else must keep the current runtime state.
  if (incoming.command == CMD_SET || incoming.command == CMD_SAVE) {
    modbus.role = incoming.role;
    modbus.address = incoming.address;
    modbus.baud = incoming.baud;
    modbus.dataBits = incoming.dataBits;
    modbus.parity = incoming.parity;
    modbus.stopBits = incoming.stopBits;
    modbus.txPin = incoming.txPin;
    modbus.rxPin = incoming.rxPin;
    modbus.dePin = incoming.dePin;
    modbus.command = incoming.command;
  } else if (incoming.command == CMD_WRITE) {
    modbus.point = incoming.point;
    modbus.value = incoming.value;
    memcpy(modbus.text, incoming.text, sizeof(modbus.text));
    modbus.command = CMD_WRITE;
  }

  tasks[KEY_MODBUS] = 1;
}

static void sendProfile() {
  if (wsClientCount() > 0) wsSendAll((uint8_t*)&modbusProfile, sizeof(modbusProfile));
}

static void handleProfileCommand() {
  if (!tasks[KEY_MODBUSPROFILE]) return;
  tasks[KEY_MODBUSPROFILE] = 0;

  if (incomingProfile &&
      (incomingProfile->command == CMD_SET || incomingProfile->command == CMD_SAVE)) {
    if (validateModbusProfile(*incomingProfile)) {
      modbusProfile = *incomingProfile;
      modbus.varCount = modbusProfile.varCount;
      resetMaster();
      if (incomingProfile->command == CMD_SAVE) saveModbusProfile(modbusProfile);
    }
  }

  if (incomingProfile) {
    delete incomingProfile;
    incomingProfile = nullptr;
  }
  modbusProfile.command = 0;
  sendProfile();
}

void onWsEventModbusProfile(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || task != KEY_MODBUSPROFILE) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (info->len != sizeof(ModbusProfile)) return;

  uint8_t* buffer = modbusProfileBuffer();
  if (!buffer) return;
  if (data != buffer) {
    memcpy(buffer, data, sizeof(ModbusProfile));
  }
  incomingProfile->key = KEY_MODBUSPROFILE;
  tasks[KEY_MODBUSPROFILE] = 1;
}

void loopModbus(uint32_t now) {
  handleProfileCommand();
  handleCommand();
  loopLocalSources(now);
  loopRaw(now);

  if (modbus.role == MODBUS_ROLE_SLAVE && uartStarted) {
    while (Serial1.available() > 0) {
      const int value = Serial1.read();
      if (value < 0) break;

      if (rxLen < MODBUS_MAX_FRAME) {
        rxFrame[rxLen++] = (uint8_t)value;
      } else {
        rxLen = 0;
        modbus.errors++;
        modbus.lastError = MODBUS_ERR_FRAME;
      }
      lastByte = now;
    }

    if (rxLen > 0 && (uint32_t)(now - lastByte) >= MODBUS_FRAME_GAP_MS) {
      processFrame(rxFrame, rxLen);
      rxLen = 0;
    }
  } else if (modbus.role == MODBUS_ROLE_MASTER && uartStarted && !rawBusy()) {
    loopMaster(now);
  }

  if (wsClientCount() > 0 && (statusDirty || (uint32_t)(now - lastStatus) >= 5000)) {
    lastStatus = now;
    statusDirty = false;
    sendModbus();
  }
}
