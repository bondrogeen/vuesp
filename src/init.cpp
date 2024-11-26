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

uint8_t readFile(const char* path, uint8_t* buf, size_t size) {
  File file = LittleFS.open(path, "r");
  if (!file) {
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
  File file = LittleFS.open(path, "w");
  file.write(buf, size);
  delay(1);
  file.close();
}

bool createDir(const char* path) {
  return LittleFS.mkdir(path);
}

void setupInit() {
  initSerial();
  initFS();
  initEeprom();
  getInfo();
  loadConfig(settings);
  initWiFi();
}