#define DEF_EEPROM_WRITE_TIME 30
#define DEF_EEPROM_SIZE 4096

#define CONFIG_START 32
#define INDEX_START_ADRESS 0

// Default struct settings
#define CONFIG_VERSION { 15,1,20 }

#define DEF_SERVER_URL "192.168.0.1"
#define DEF_SERVER_PORT 3001

#define DEF_ETH_DHCP 1
#define DEF_ETH_IP { 192,168,1,11 }
#define DEF_ETH_SUBNET { 255,255,255,0 }
#define DEF_ETH_GETEWAY { 192,168,1,1 }
#define DEF_ETH_DNS { 8,8,8,8 }

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP { 192,168,1,12 }
#define DEF_WIFI_SUBNET { 255,255,255,0 }
#define DEF_WIFI_GETEWAY { 192,168,1,1 }
#define DEF_WIFI_DNS { 8,8,8,8 }

// #define DEF_WIFI_MODE WIFI_STA
#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID "ESP-8266"
#define DEF_WIFI_PASS ""

#define DEF_HTTP_MODE 1
#define DEF_HTTP_LOGIN "admin"
#define DEF_HTTP_PASS "admin"

#define DEF_DEVICE_NAME "esp-1"

#define DEF_DEVICE_FIRMWARE {0,0,6}

enum ws_comm {
  OFF,
  ON,
  ERASE,
  SETTINGS,
  SAVE,
  REBOOT,
  INFO,
  PING,
  SCAN,
  PROGRESS,
  FILES,
};


struct StoreStruct {
  uint8_t init;
  uint8_t version[3];
  char serverUrl[32];
  uint16_t serverPort;

  uint8_t ethDhcp;
  uint8_t ethIp[4];
  uint8_t ethSubnet[4];
  uint8_t ethGeteway[4];
  uint8_t ethDns[4];

  uint8_t wifiDhcp;
  uint8_t wifiIp[4];
  uint8_t wifiSubnet[4];
  uint8_t wifiGeteway[4];
  uint8_t wifiDns[4];

  uint8_t wifiMode;
  char wifiSsid[32];
  char wifiPass[32];

  uint8_t httpMode;
  char httpLogin[12];
  char httpPass[12];

  char deviceName[12];
} storage = {
    SETTINGS,
    CONFIG_VERSION,
    DEF_SERVER_URL,
    DEF_SERVER_PORT,
    DEF_ETH_DHCP,
    DEF_ETH_IP,
    DEF_ETH_SUBNET,
    DEF_ETH_GETEWAY,
    DEF_ETH_DNS,
    DEF_WIFI_DHCP,
    DEF_WIFI_IP,
    DEF_WIFI_SUBNET,
    DEF_WIFI_GETEWAY,
    DEF_WIFI_DNS,
    DEF_WIFI_MODE,
    DEF_WIFI_SSID,
    DEF_WIFI_PASS,
    DEF_HTTP_MODE,
    DEF_HTTP_LOGIN,
    DEF_HTTP_PASS,
    DEF_DEVICE_NAME
    };


struct Info {
  uint8_t init;
  uint8_t firmware[3];
  uint32_t totalBytes;
  uint32_t usedBytes;
} info_fs = {
  INFO,
  DEF_DEVICE_FIRMWARE,
  0,
  0
};

struct Ping {
  uint8_t init;
} ping = {
  PING
};

struct Files {
  uint8_t init;
  uint8_t isDir;
  uint8_t isFile;
  uint8_t command;
  char name[32];
  uint32_t size;
} files = {
  FILES,
  0,
  0,
  0,
  "",
  0
};

struct Scan {
  uint8_t init;
  uint8_t id;
  char name[32];
} scan = {
  SCAN,
  0,
  "",
};

struct Progress {
  uint8_t init;
  uint8_t status;
  uint16_t empty;
  uint32_t size;
  uint32_t length;
} progress = {
  PROGRESS,
  0,
  0,
  0,
  0
};