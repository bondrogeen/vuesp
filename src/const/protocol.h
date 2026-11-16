#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

enum key : uint8_t {
  KEY_INIT = 0,
  KEY_SETTINGS = 1,
  KEY_INFO = 2,
  KEY_PING = 3,
  KEY_SCAN = 4,
  KEY_PROGRESS = 5,
  KEY_FILES = 6,
  KEY_REBOOT = 7,
  KEY_EVENT = 8,
  KEY_PORT = 9,
  KEY_MESSAGE = 10,
  KEY_DALLAS = 11,
  KEY_BUFFER = 12,
  KEY_DEVICE = 13,
  KEY_DISCOVERY = 14,
  KEY_END
};

enum cmd : uint8_t {
  CMD_NONE = 0,
  CMD_GET = 1,
  CMD_SET = 2,
  CMD_GET_ALL = 3,
  CMD_CHANGE = 4,
  CMD_RESTORE = 5,
  CMD_READ = 6,
  CMD_WRITE = 7,
  CMD_SAVE = 254,
  CMD_END
};

enum portMode : uint16_t {
  PORT_NONE = 0,
  PORT_INPUT = 1 << 0,
  PORT_INPUT_PULLUP = 1 << 1,
  PORT_OUTPUT = 1 << 2,
  PORT_OUTPUT_OPEN_DRAIN = 1 << 3,
  PORT_PWM = 1 << 4,
  PORT_ONEWIRE = 1 << 5,
  PORT_ADC = 1 << 6,
  PORT_DAC = 1 << 7
};

#pragma pack(push, 1)

struct Init {
  uint8_t key;
};
struct Settings {
  uint8_t key;
  uint8_t wifiDhcp;
  uint8_t wifiMode;
  uint8_t authMode;
  uint16_t version;
  uint16_t device;

  uint8_t discovery;
  uint8_t discoveryInterval;
  uint16_t discoveryPort;

  uint8_t wifiIp[4];
  uint8_t wifiSubnet[4];
  uint8_t wifiGateway[4];
  uint8_t wifiDns[4];

  char wifiSsid[32];
  char wifiPass[32];
  char authLogin[12];
  char authPass[12];
};

struct Info {
  uint8_t key;
  uint8_t firmware[3];
  uint32_t totalBytes;
  uint32_t usedBytes;
  uint32_t id;
  uint64_t uptime;
  char name[20];
  uint32_t board;
};

struct Ping {
  uint8_t key;
};

struct Scan {
  uint8_t key;
  uint8_t id;
  uint8_t isHidden;
  uint8_t encryptionType;
  int32_t channel;
  int32_t rssi;
  char ssid[32];
};

struct Progress {
  uint8_t key;
  uint8_t status;
  uint16_t empty;
  uint32_t size;
  uint32_t length;
};

struct Files {
  uint8_t key;
  uint8_t isDir;
  uint8_t isFile;
  uint8_t command;
  uint32_t size;
  char name[32];
};

struct Event {
  uint8_t key;
  uint8_t command;
  uint16_t subCommand;
  uint32_t value1;
  uint32_t value2;
  char name[32];
};

struct Port {
  uint8_t key;
  uint8_t gpio;
  uint16_t value;
  uint8_t mode;
  uint8_t interrupt;
  uint16_t list;
  uint8_t disabled;
  uint8_t isButton;
  uint8_t command;
  uint8_t isPressed;
  uint8_t count;
  uint8_t empty;
  uint16_t valueOld;
  uint32_t pressStart;
};

struct Message {
  uint8_t key;
  uint8_t type;
  uint8_t id;
  uint8_t active;
  uint8_t handler;
  uint8_t index;
  uint16_t len;
  uint32_t date;
  char text[256];
};

struct Dallas {
  uint8_t key;
  uint8_t command;
  uint16_t subCommand;
  uint8_t address[8];
  float temp;
};

struct Buffer {
  uint8_t key;
  uint8_t empty;
  uint16_t head;
  uint16_t tail;
  uint16_t count;
  int8_t data[256];
};
struct Discovery {
  uint8_t key;
  uint8_t command;
  uint8_t status;
  uint8_t empty;
  uint32_t id;
  uint32_t ip;
  uint32_t lastSeen;
};

struct Device {
  uint8_t key;
  uint8_t command;
  uint16_t analog;
  uint32_t now;
  uint32_t list;
  char message[32];
};
#pragma pack(pop)

#include "protocol.generated.h"

#endif
