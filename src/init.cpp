#include "./include/init.h"

char nameDevice[13];

#if defined(ESP8266)
uint32_t id = ESP.getChipId();
#elif defined(ESP32)
uint32_t id = (uint32_t)(ESP.getEfuseMac() >> 24);
#endif

// WiFiClient WiFIclient;

Info infoFS = {KEY_INFO, DEF_DEVICE_FIRMWARE, 0, 0, 0};

Settings settings = {
    KEY_SETTINGS,
    DEF_WIFI_DHCP,
    DEF_WIFI_MODE,
    DEF_AUTH_MODE,
    CONFIG_VERSION,
    0,
    DEF_WIFI_IP,
    DEF_WIFI_SUBNET,
    DEF_WIFI_GETEWAY,
    DEF_WIFI_DNS,
    DEF_WIFI_SSID,
    DEF_WIFI_PASS,
    DEF_AUTH_LOGIN,
    DEF_AUTH_PASS};

void reboot() {
  ESP.restart();
}

void saveEeprom(Settings& settings) {
  EEPROM.put(CONFIG_START, settings);
  EEPROM.commit();
}

void saveSettings(Settings& settings) {
  EEPROM.put(CONFIG_START, settings);
  EEPROM.commit();
}

void loadConfig(Settings& settings) {
  uint16_t version = 345;
  EEPROM.get(CONFIG_START + 4, version);
  if (version == settings.version) {
    EEPROM.get(CONFIG_START, settings);
  } else {
    strcpy(settings.wifiSsid, nameDevice);
    saveSettings(settings);
  }
}

void getInfo() {
  sprintf(nameDevice, "%s%02X", DEF_DEVICE_NAME, id);
#if defined(ESP8266)
  FSInfo fs_info;
  LittleFS.info(fs_info);
  infoFS.totalBytes = fs_info.totalBytes;
  infoFS.usedBytes = fs_info.usedBytes;
#elif defined(ESP32)
  infoFS.totalBytes = LittleFS.totalBytes();
  infoFS.usedBytes = LittleFS.usedBytes();
#endif
  infoFS.id = id;
  return;
}

void initFS() {
  if (!LittleFS.begin(FORMAT_LITTLEFS_IF_FAILED) && LittleFS.format()) Serial.println(F("Filesystem formatted!"));
}

void initEeprom() {
  EEPROM.begin(256);
}

void initSerial() {
  Serial.begin(115200);
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  if (event == 7) {
    Serial.println(WiFi.localIP());
  }
}

void initWiFi() {
  if (settings.wifiMode) {
    WiFi.setHostname(nameDevice);
    WiFi.mode((WiFiMode_t)settings.wifiMode);
    if (!settings.wifiDhcp) WiFi.config(settings.wifiIp, settings.wifiGeteway, settings.wifiSubnet, settings.wifiDns);
    if (settings.wifiMode == WIFI_STA) WiFi.begin(settings.wifiSsid, settings.wifiPass);
    if (settings.wifiMode == WIFI_AP) WiFi.softAP(settings.wifiSsid, settings.wifiPass);
    WiFi.onEvent(WiFiEvent);
  }
}

void listDir(fs::FS& fs, const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");

#ifdef CONFIG_LITTLEFS_FOR_IDF_3_2
      Serial.println(file.name());
#else
      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
#endif

      if (levels) {
        listDir(fs, file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");

#ifdef CONFIG_LITTLEFS_FOR_IDF_3_2
      Serial.println(file.size());
#else
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
#endif
    }
    file = root.openNextFile();
  }
}
void listDir(const char* dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = LittleFS.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");

      Serial.print(file.name());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);

      if (levels) {
        listDir(file.name(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");

      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm* tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, (tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour, tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

uint8_t readFile(const char* path, uint8_t* buf, size_t size) {
  Serial.println(path);
  Serial.println("readFile");
  File file = LittleFS.open(path, "r");
  Serial.println(file);
  if (!file || file.isDirectory()) {
    return 0;
  } else {
    while (file.available()) {
      file.read(buf, size);
    }
    file.close();
    return 1;
  }
}

void writeFile(const char* path, const uint8_t* buf, size_t size) {
  File file = LittleFS.open(path, FILE_WRITE);
  file.write(buf, size);
  delay(1);
  file.close();
}

bool createDir(const char* path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (LittleFS.mkdir(path)) {
    Serial.println("Dir created");
    return 1;
  } else {
    Serial.println("mkdir failed");
    return 0;
  }
}

void writeFile(const char* path, const char* message) {
  Serial.printf("Writing file: %s\r\n", path);

  File file = LittleFS.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("- failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
  file.close();
}

void setupInit() {
  initSerial();
  initEeprom();
  initFS();
  getInfo();
  loadConfig(settings);
  initWiFi();

  // listDir("/", 0);
  // createDir("/temp");
  // writeFile("/temp/hello2.txt", "Hello2");
}