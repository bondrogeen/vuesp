#define CONFIG_START 0

// Default struct settings
#define CONFIG_VERSION 4

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP \
  { 192, 168, 1, 12 }
#define DEF_WIFI_SUBNET \
  { 255, 255, 255, 0 }
#define DEF_WIFI_GETEWAY \
  { 192, 168, 1, 1 }
#define DEF_WIFI_DNS \
  { 8, 8, 8, 8 }

#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID "VUESP-"
#define DEF_WIFI_PASS ""

#define DEF_AUTH_MODE 1
#define DEF_AUTH_LOGIN "admin"
#define DEF_AUTH_PASS "admin"

#define DEF_DEVICE_FIRMWARE \
  { 0, 0, 6 }

#define RES_TYPE_JSON "application/json"

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
  END
};

struct Info {
  uint8_t init;
  uint8_t firmware[3];
  uint32_t totalBytes;
  uint32_t usedBytes;
  uint32_t id;
} ;

struct Ping {
  uint8_t init;
};

struct Files {
  uint8_t init;
  uint8_t isDir;
  uint8_t isFile;
  uint8_t command;
  uint32_t size;
  char name[32];
};

struct Scan {
  uint8_t init;
  uint8_t id;
  uint8_t isHidden;
  uint8_t encryptionType;
  int32_t channel;
  int32_t rssi;
  char ssid[32];
};

struct Progress {
  uint8_t init;
  uint8_t status;
  uint16_t empty;
  uint32_t size;
  uint32_t length;
};