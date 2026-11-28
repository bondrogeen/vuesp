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
  KEY_MODBUS = 15,
  KEY_MODBUSPROFILE = 16,
  KEY_MODBUSRAW = 17,
  KEY_SCRIPTCMD = 18,
  KEY_SCRIPTSTATUS = 19,
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

enum scriptAct : uint8_t {
  SCRIPTACT_STATUS = 0,
  SCRIPTACT_RUN = 1,
  SCRIPTACT_STOP = 2,
  SCRIPTACT_STOP_ALL = 3,
  SCRIPTACT_REMOVE = 4,
  SCRIPTACT_SAVE = 5,
  SCRIPTACT_END
};

enum scriptRes : uint8_t {
  SCRIPTRES_OK = 0,
  SCRIPTRES_NOT_FOUND = 1,
  SCRIPTRES_INVALID = 2,
  SCRIPTRES_BUSY = 3,
  SCRIPTRES_IO = 4,
  SCRIPTRES_CRC = 5,
  SCRIPTRES_VERIFY = 6,
  SCRIPTRES_NO_SPACE = 7,
  SCRIPTRES_NO_CACHE = 8,
  SCRIPTRES_END
};

enum scriptState : uint8_t {
  SCRIPT_STATE_ACTIVE = 0x01,
  SCRIPT_STATE_LOADED = 0x02,
  SCRIPT_STATE_ERROR = 0x04,
  SCRIPT_STATE_BOOT = 0x08
};

// Firmware-only flag: the web triggers a run with the HTTP query run=1.
#define SCRIPT_FLAG_RUN 0x01

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

enum modbusLimits : uint16_t {
  MODBUS_DEVICES_MAX = 8,
  MODBUS_POINTS_MAX = 64,
  MODBUS_VARS_MAX = 32
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

  uint32_t freeHeap;
  uint32_t minFreeHeap;
  uint32_t maxBlock;
  uint32_t stackFree;
  uint32_t loopLastUs;
  uint32_t loopMaxUs;
  uint32_t loopP95Us;
  uint32_t loopCount;
  uint16_t fragmentation;
  uint8_t clients;
  uint8_t scripts;
  uint8_t peers;
  uint8_t wifi;
  uint8_t diagnostics;
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

struct ModbusVarValue {
  uint8_t var;
  uint8_t quality;
  uint8_t flags;
  uint8_t reserved;
  double value;
  char text[16];
};

struct ModbusDevice {
  char id[12];
  char name[20];
  uint8_t address;
  uint8_t enabled;
  uint16_t interval;
  uint16_t timeout;
  uint8_t retries;
};

struct ModbusPoint {
  uint8_t device;
  char id[12];
  char name[20];
  uint8_t fc;
  uint16_t address;
  uint8_t type;
  uint8_t flags;
  uint8_t registers;
  uint8_t decimals;
  float scale;
  float offset;
  char unit[8];
  uint8_t var;
};

struct ModbusVar {
  char id[16];
  char name[20];
  uint8_t type;
  uint8_t flags;
  char unit[8];
  uint8_t decimals;
  uint8_t point;
  char source[32];
};

struct ModbusProfile {
  uint8_t key;
  uint8_t command;
  uint16_t version;
  uint8_t deviceCount;
  uint8_t pointCount;
  uint8_t varCount;
  uint8_t reserved;
  ModbusDevice devices[MODBUS_DEVICES_MAX];
  ModbusPoint points[MODBUS_POINTS_MAX];
  ModbusVar vars[MODBUS_VARS_MAX];
};

struct ModbusRaw {
  uint8_t key;
  uint8_t command;
  uint8_t slave;
  uint8_t function;
  uint16_t address;
  uint8_t length;
  uint8_t autoCrc;
  uint8_t request[64];
  uint8_t requestLen;
  uint8_t response[64];
  uint8_t responseLen;
  uint8_t status;
  uint8_t reserved;
  uint32_t elapsed;
};

struct Modbus {
  uint8_t key;
  uint8_t command;
  uint8_t role;  // 0 off, 1 slave, 2 master
  uint8_t address;
  uint32_t baud;
  uint8_t dataBits;
  uint8_t parity;
  uint8_t stopBits;
  uint8_t txPin;
  uint8_t rxPin;
  int8_t dePin;
  uint8_t status;  // 0 off, 1 listening/polling, 2 error
  uint8_t lastFunction;
  uint8_t lastAddress;
  uint8_t lastError;
  uint32_t requests;
  uint32_t errors;
  uint8_t varCount;
  uint8_t reserved[3];
  uint32_t updated;
  uint8_t point;
  uint8_t ok;
  uint8_t pad[2];
  double value;
  char text[16];
  ModbusVarValue vars[MODBUS_VARS_MAX];
};

// vuesp-struct matches KEY_SCRIPTCMD to the struct name case-insensitively,
// so this type must stay "ScriptCmd" (not ScriptCommand).
struct ScriptCmd {
  uint8_t key;
  uint8_t action;
  uint8_t flags;
  uint16_t requestId;
  char name[16];
};

struct ScriptStatus {
  uint8_t key;
  uint8_t result;
  uint8_t state;
  uint8_t packError;
  uint16_t requestId;
  uint8_t error;
  uint16_t errorPc;
  char name[16];
};

#pragma pack(pop)

#include "protocol.generated.h"

#endif
