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
  uint8_t gpio12;
  uint8_t gpio14;
  uint16_t analog;
  uint16_t pwm;
  uint32_t now;
  char message[32];
};
