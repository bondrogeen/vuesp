enum keys {
  KEY_INIT,
  KEY_SETTINGS,
  KEY_INFO,
  KEY_PING,
  KEY_SCAN,
  KEY_PROGRESS,
  KEY_FILES,
  KEY_REBOOT,
  KEY_PORT,
  KEY_DEVICE,
  KEY_DALLAS,
  KEY_NOTIFICATION,
  KEY_BUFFER,
  KEY_END
};

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
  uint32_t uptime;
  char name[20];
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

struct Port {
  uint8_t key;
  uint8_t gpio;
  uint8_t mode;
  uint8_t interrupt;
  uint16_t value;
  uint8_t state;
  uint8_t command;
};

struct Notification {
  uint8_t key;
  uint8_t isNew;
  uint8_t color;
  uint8_t timeout;
  uint32_t date;
  char text[128];
};

struct Dallas {
  uint8_t key;
  uint8_t comm;
  uint16_t comm2;
  uint8_t address[8];
  float temp;
};

struct Device {
  uint8_t key;
  uint8_t command;
  uint16_t voltage;
  uint16_t current;
  uint16_t balanceCapacity;
  uint16_t rateCapacity;
  uint16_t cycle;
  uint16_t empty;
  uint16_t balanceStatus;
  uint16_t balanceStatusHigh;
  uint16_t protectionStatus;
  uint8_t version;
  uint8_t rsoc;
  uint8_t fet;
  uint8_t series;
  uint8_t temp;
  uint8_t ntc;
  uint16_t ntc1;
  uint16_t ntc2;
  uint16_t ntc3;
  uint16_t cellLow;
  uint16_t cellHigh;
  uint16_t cellDiff;
  uint16_t cellAvg;
  uint8_t dateDay;
  uint8_t dateMonth;
  uint16_t dateYear;
  uint16_t cellVoltage[14];
};
