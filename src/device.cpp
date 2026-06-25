#include "./include/device.h"

#include <Wire.h>

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Device device = {KEY_DEVICE, 0, 6};
uint32_t lastTimeDevice = 0;
uint8_t response[BMS_LEN_RESPONSE];

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
void deviceGPIO(Port* port) {
  Serial.print(port->gpio);
  Serial.println(port->value);
}

void deviceGPIOInterrupt() {
}

void getADC() {
  // device.analog = analogRead(A0);
}

void getData() {
  getADC();
}

void setupDevice() {
  Serial1.begin(9600, SERIAL_8N1, 5, 6);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

uint16_t combineBytes(uint8_t high_byte, uint8_t low_byte) {
  return (uint16_t)((high_byte << 8) | low_byte);
}

void readBmsData() {
  uint8_t reqMessage[] = {0xDD, 0xA5, 0x03, 0x00, 0xFF, 0xFD, 0x77};
  Serial1.write(reqMessage, 7);
}

void readPackData() {
  uint8_t reqMessage[] = {0xDD, 0xA5, 0x04, 0x00, 0xFF, 0xFC, 0x77};
  Serial1.write(reqMessage, 7);
}

void parsDate(uint16_t code) {
  device.dateDay = code & 0x1F;
  device.dateMonth = (code >> 5) & 0x0F;
  device.dateYear = 2000 + (code >> 9);
}

void parseReqBasicMessage(uint8_t* message) {
  device.voltage = combineBytes(message[4], message[5]);
  device.current = combineBytes(message[6], message[7]);
  device.balanceCapacity = combineBytes(message[8], message[9]);
  device.rateCapacity = combineBytes(message[10], message[11]);
  device.cycle = combineBytes(message[12], message[13]);
  parsDate(combineBytes(message[14], message[15]));
  device.balanceStatus = combineBytes(message[16], message[17]);
  device.balanceStatusHigh = combineBytes(message[18], message[19]);
  device.protectionStatus = combineBytes(message[20], message[21]);
  device.version = message[22];
  device.rsoc = message[23];
  device.fet = message[24];
  device.series = message[25];
  device.ntc = message[26];
  device.ntc1 = (combineBytes(message[27], message[28]) - 2731) * 10;
  if (device.ntc > 1) {
    device.ntc2 = (combineBytes(message[29], message[30]) - 2731) * 10;
  }
  if (device.ntc > 2) {
    device.ntc3 = (combineBytes(message[31], message[32]) - 2731) * 10;
  }
}

void parseReqPackMessage(uint8_t* message) {
  uint8_t numOfCells = message[3] / 2;
  uint16_t _cellSum = 0;
  uint16_t _cellLow = 5000;  // 5v
  uint16_t _cellHigh = 0;

  byte offset = 4;
  for (byte i = 0; i < numOfCells; i++) {
    device.cellVoltage[i] = ((uint16_t)combineBytes(message[i * 2 + offset], message[i * 2 + 1 + offset]));
    _cellSum += device.cellVoltage[i];

    if (device.cellVoltage[i] > _cellHigh) {
      _cellHigh = device.cellVoltage[i];
    }
    if (device.cellVoltage[i] < _cellLow) {
      _cellLow = device.cellVoltage[i];
    }

    device.cellLow = _cellLow;
    device.cellHigh = _cellHigh;
    device.cellDiff = _cellHigh - _cellLow;
    device.cellAvg = _cellSum / numOfCells;
  }
}

bool verifyJBDPacket(const uint8_t* packet, uint8_t len) {
  if (len < 7 || packet[0] != 0xDD || packet[len - 1] != 0x77) return false;
  uint16_t sum = 0;
  for (uint8_t i = 2; i < len - 3; i++) {
    sum += packet[i];
  }
  uint16_t calc = ~sum + 1;
  uint16_t recv = (packet[len - 3] << 8) | packet[len - 2];
  return calc == recv;
}

bool readResponse(uint8_t* message) {
  if (!Serial1.available()) return false;
  uint8_t i = 0;
  bool found = false;
  uint32_t timeout = millis() + 100;

  while (Serial1.available() || millis() < timeout) {
    if (Serial1.available()) {
      uint8_t b = Serial1.read();
      // if (b < 0x10) {
      //   Serial.print("0");
      // }
      // Serial.print(b, HEX);
      // Serial.print(" ");
      if (b == 0xDD) found = true;
      if (found) {
        message[i++] = b;
        if (i >= 128) break;
      }
      timeout = millis() + 100;
    }
  }
  if (!found || i < 5) return false;
  if (message[i - 1] != 0x77) return false;
  if (!verifyJBDPacket(message, i)) return false;
  return true;
}

uint8_t receive = 1;

void loopDevice(uint32_t now) {
  if (readResponse(response)) {
    // Serial.println(response[1]);
    if (response[1] == 3) {
      parseReqBasicMessage(response);
    }
    if (response[1] == 4) {
      parseReqPackMessage(response);
    }
    onSendDevice();
  }

  if (now - lastTimeDevice > 2000) {
    lastTimeDevice = now;
    if (receive == 1) {
      readBmsData();
      receive = 0;
    } else {
      receive = 1;
      readPackData();
    }
    Serial.println(settings.wifiMode);
    Serial.println(settings.wifiSsid);
    Serial.println(settings.wifiPass);
    Serial.println(device.dateYear);
    // Serial.println(device.cellHigh, DEC);
    // Serial.println(device.cellVoltage[0], DEC);
    // Serial.println(device.cellVoltage[1], DEC);
    // Serial.println(device.cellVoltage[2], DEC);
    // Serial.println(device.cellVoltage[3], DEC);
    // Serial.println(device.cellVoltage[4], DEC);
  }

  if (tasks[KEY_DEVICE]) {
    // if (device.command == DEVICE_COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    // if (device.command == DEVICE_COMMAND_TEXT) sendNotification();

    // device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}
