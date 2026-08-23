#include "./device.h"

Device device = {KEY_DEVICE, 0, 6};
uint32_t lastTimeDevice = 0;
char response[BMS_LEN_RESPONSE];

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
  // Serial.print(port->gpio);
  // Serial.println(port->value);
}

// void sendNotification(const char* text) {
//   message.type = MESSAGE_TYPE_NOTIFICATION;
//   memset(message.text, 0, sizeof(message.text));
//   strcpy(message.text, text);
//   wsSendAll((uint8_t*)&message, sizeof(message));
// }

void setupDevice() {
  Serial1.begin(9600, SERIAL_8N1, 5, 6);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void sendCommand11() {
  // ~120146110000FDB0
  byte cmd[] = {0x7E, 0x31, 0x32, 0x30, 0x31, 0x34, 0x36, 0x31, 0x31, 0x30, 0x30, 0x30, 0x30, 0x46, 0x44, 0x42, 0x30, 0x0D};
  Serial1.write(cmd, sizeof(cmd));
}

void sendCommandVersion() {
  // ~120146B00000FDA0
  byte cmd[] = {0x7E, 0x31, 0x32, 0x30, 0x31, 0x34, 0x36, 0x42, 0x30, 0x30, 0x30, 0x30, 0x30, 0x46, 0x44, 0x41, 0x30, 0x0D};
  Serial1.write(cmd, sizeof(cmd));
}

void sendCommandB1() {
  // ~120146B10000FD9F
  byte cmd[] = {0x7E, 0x31, 0x32, 0x30, 0x31, 0x34, 0x36, 0x42, 0x31, 0x30, 0x30, 0x30, 0x30, 0x46, 0x44, 0x39, 0x46, 0x0D};
  Serial1.write(cmd, sizeof(cmd));
}

void sendCommandMain() {
  // ~12014631E00201FD36
  byte cmd[] = {0x7E, 0x31, 0x32, 0x30, 0x31, 0x34, 0x36, 0x33, 0x31, 0x45, 0x30, 0x30, 0x32, 0x30, 0x31, 0x46, 0x44, 0x33, 0x36, 0x0D};
  Serial1.write(cmd, sizeof(cmd));
}

void sendCommandAlarm() {
  // ~12014633E00201FD34
  byte cmd[] = {0x7E, 0x31, 0x32, 0x30, 0x31, 0x34, 0x36, 0x33, 0x33, 0x45, 0x30, 0x30, 0x32, 0x30, 0x31, 0x46, 0x44, 0x33, 0x34, 0x0D};
  Serial1.write(cmd, sizeof(cmd));
}

int32_t parseHexValue(char* data, uint16_t start, uint8_t len) {
  if (len == 0 || len > 8) return 0;
  int32_t res = 0;
  for (uint8_t i = 0; i < len; i++) {
    char c = (char)data[start + i];
    res = (res << 4) | (c <= '9' ? c - '0' : (c & 0x0F) + 9);
  }
  if (len < 8 && (res & (1 << (len * 4 - 1)))) {
    res |= ~((1 << (len * 4)) - 1);
  }
  return res;
}

// ~120046003058000104  (0-18)
// 0D0E - cell1 (19-22)
// 0D0E - cell2 (23-26)
// 0D09 - cell3 (27-30)
// 0D07 - cell4 (31-34)
// 04 - C_Count (35-36)
// 0BBB - temp1 (37-40)
// 0BBD - temp2 (41-44)
// 0BC4 - temp3 (45-48)
// 0BA4 - temp4 (49-52)
// 0000 - reserve (53-56)
// 03EA - current (57-60)
// 0000 - reserve (61-64)
// 342C - voltage (65-68)
// 0000 - reserve (69-72)
// 16E2  - capacity (73-76)
// 00 - reserve (77-78)
// 0000 - reserve (79-82)
// 2710 - Fcc_raw (83-86)
// 0001 - Cycle (87-90)
// 0000 - reserve (91-94)
// 2710 - Dcc_raw (95-98)
// 3B - SOC (99-100) = 59%
// EB3F - CRC (101-104)

void parseMessageMain(char* message) {
  device.cell1 = parseHexValue(message, 19, 4);
  device.cell2 = parseHexValue(message, 23, 4);
  device.cell3 = parseHexValue(message, 27, 4);
  device.cell4 = parseHexValue(message, 31, 4);
  device.temp1 = parseHexValue(message, 37, 4) - 2730;
  device.temp2 = parseHexValue(message, 41, 4) - 2730;
  device.temp3 = parseHexValue(message, 45, 4) - 2730;
  device.temp4 = parseHexValue(message, 49, 4) - 2730;
  device.current = parseHexValue(message, 53, 8);
  device.voltage = parseHexValue(message, 65, 4);
  device.capacity = parseHexValue(message, 73, 4);
  device.fccRaw = parseHexValue(message, 83, 4);
  device.cycle = parseHexValue(message, 87, 4);
  device.dccRaw = parseHexValue(message, 95, 4);
  device.SOC = parseHexValue(message, 99, 2);
}

// ~12004600D0300001 (0-16)
// 04 - cellCount (17-18)
// 00 - cellStatus1 (19-20)
// 00 - cellStatus2 (21-22)
// 00 - cellStatus3 (23-24)
// 00 - cellStatus4 (25-26)
// 04 - tempCount (27-28)
// 00 - tempStatus1 (29-30)
// 00 - tempStatus2 (31-32)
// 00 - tempStatus3 (33-34)
// 00 - tempStatus4 (35-36)
// 00 - chgCurrStatus (37-38)
// 00 - totalVoltStatus (39-40)
// 00 - dsgCurrStatus (41-42)
// 00 - status1 (43-44)
// 00 - status2 (45-46)
// 26 - status3 (47-48)
// 00 - status4 (49-50)
// 00 - status5 (51-52)
// 00 - status6 (53-54)
// 00 - status7 (55-56)
// 00 - status8 (57-58)
// 00 - status9 (59-60)
// F48B - CRC

void parseMessageAlarm(char* message) {
  device.cellCount = parseHexValue(message, 17, 2);
  device.cellStatus1 = parseHexValue(message, 19, 2);
  device.cellStatus2 = parseHexValue(message, 21, 2);
  device.cellStatus3 = parseHexValue(message, 23, 2);
  device.cellStatus4 = parseHexValue(message, 25, 2);

  device.tempCount = parseHexValue(message, 27, 2);
  device.tempStatus1 = parseHexValue(message, 29, 2);
  device.tempStatus2 = parseHexValue(message, 31, 2);
  device.tempStatus3 = parseHexValue(message, 33, 2);
  device.tempStatus4 = parseHexValue(message, 35, 2);

  device.chgCurrStatus = parseHexValue(message, 37, 2);
  device.totalVoltStatus = parseHexValue(message, 39, 2);
  device.dsgCurrStatus = parseHexValue(message, 41, 2);

  device.status1 = parseHexValue(message, 43, 2);
  device.status2 = parseHexValue(message, 45, 2);
  device.status3 = parseHexValue(message, 47, 2);
  device.status4 = parseHexValue(message, 49, 2);
  device.status5 = parseHexValue(message, 51, 2);
  device.status6 = parseHexValue(message, 53, 2);
  device.status7 = parseHexValue(message, 55, 2);
  device.status8 = parseHexValue(message, 57, 2);
  device.status9 = parseHexValue(message, 59, 2);
}
// ~12004600D0300001 (0-16)
// ~12004600103C303937332D30303356312E30302D303030302D4630343130300000000000F199

void parseMessageVersion(char* message) {
  int j = 0;
  for (int i = 0; i < 60; i += 2) {
    char hex[3] = {message[13 + i], message[13 + i + 1], 0};
    device.version[j++] = (char)strtol(hex, NULL, 16);
  }
}

uint8_t hexToByte(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

bool checkFrame(char* data, uint16_t len) {
  uint16_t crcPacket = 0;
  for (uint8_t i = 0; i < 4; i++) {
    uint8_t index = len - 5 + i;
    char c = (char)data[index];
    uint8_t digit;
    if (c >= '0' && c <= '9')
      digit = c - '0';
    else if (c >= 'A' && c <= 'F')
      digit = c - 'A' + 10;
    else if (c >= 'a' && c <= 'f')
      digit = c - 'a' + 10;
    else
      return false;
    crcPacket = (crcPacket << 4) | digit;
  }
  uint32_t sum = 0;
  for (uint16_t i = 1; i < len - 5; i++) {
    sum += data[i];
  }
  uint16_t crcCalc = (uint16_t)(~(sum % 65536) + 1);

  return crcCalc == crcPacket;
}

bool readResponse(char* message) {
  if (!Serial1.available()) return false;
  uint8_t i = 0;
  bool found = false;
  uint32_t timeout = millis() + 100;

  while (Serial1.available() || millis() < timeout) {
    if (Serial1.available()) {
      uint8_t b = Serial1.read();
      Serial.print((char)b);
      if (b == 0x7E) found = true;
      if (found) {
        message[i++] = (char)b;
        if (i >= 128) break;
      }
      timeout = millis() + 100;
    }
  }
  Serial.println(" ");
  if (!found || i < 5) return false;
  if (message[i - 1] != 0x0D) return false;
  uint16_t crc = checkFrame(message, i - 1);
  if (crc != 0) return false;
  return true;
}

uint8_t receive = 0;

void loopDevice(uint32_t now) {
  if (readResponse(response)) {
    // Serial.println(response[9], DEC);
    device.lastTime = 0;
    if (response[9] == 51) {
      parseMessageMain(response);
    }
    if (response[9] == 68) {
      parseMessageAlarm(response);
    }
    if (response[9] == 49) {
      parseMessageVersion(response);
    }
    onSendDevice();
    // sendNotification("Test");
  }

  if (now - lastTimeDevice > 2000) {
    lastTimeDevice = now;
    device.lastTime = now;
    // Serial.println("test");
    // sendCommand11();
    // sendCommandB0();
    // sendCommandB1();
    // sendCommandMain();
    // sendCommandAlarm();

    if (receive == 0) {
      sendCommandVersion();
    }
    if (receive == 1) {
      sendCommandMain();
    }
    if (receive == 2) {
      sendCommandAlarm();
      device.status3 = 0;
      receive = 0;
    }
    receive++;
  }

  if (tasks[KEY_DEVICE]) {
    // if (device.command == DEVICE_COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    // if (device.command == DEVICE_COMMAND_TEXT) sendNotification();

    // device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}
