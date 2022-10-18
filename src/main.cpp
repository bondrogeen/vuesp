#include <EEPROM.h>

#include "webserver.h"

#ifdef ESP32
#include <AsyncTCP.h>
#include <FS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

Info info_fs = {INFO, DEF_DEVICE_FIRMWARE, 0, 0, 0};
Scan scan = {SCAN, 0, 0, 0, 0, 0, ""};
Files files = {FILES, 0, 0, 0, 0, ""};

Settings storage = {
    SETTINGS,
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

WiFiClient WiFIclient;

uint8_t wsTask[END];
uint32_t now;
uint8_t clientId;

#if defined(ESP8266)
FSInfo fs_info;
uint32_t id = ESP.getChipId();

#elif defined(ESP32)

uint32_t id = (uint32_t)(ESP.getEfuseMac() >> 32);
#endif

#if defined(ESP8266)
void getFile(char *name) {
  Dir dir = LittleFS.openDir(name);
  while (dir.next()) {
    memset(files.name, 0, sizeof(files.name));
    dir.fileName().toCharArray(files.name, 32);
    files.size = dir.fileSize();
    files.isFile = dir.isFile();
    files.isDir = dir.isDirectory();
    wsSend((uint8_t *)&files, sizeof(files));
  }
  wsTask[FILES] = OFF;
}

#elif defined(ESP32)

void getFile(char *name) {
  File root = LittleFS.open(name);
  File file = root.openNextFile();
  String str = "";
  while (file) {
    memset(files.name, 0, sizeof(files.name));
    strcpy(files.name, file.name());
    files.size = file.size();
    files.isFile = !file.isDirectory();
    files.isDir = file.isDirectory();
    wsSend((uint8_t *)&files, sizeof(files));
    file = root.openNextFile();
  }
  wsTask[FILES] = OFF;
}
#endif

void reboot() {
  ESP.restart();
}

void getInfo() {
#if defined(ESP8266)
  LittleFS.info(fs_info);
  info_fs.totalBytes = fs_info.totalBytes;
  info_fs.usedBytes = fs_info.usedBytes;
#elif defined(ESP32)
  info_fs.totalBytes = LittleFS.totalBytes();
  info_fs.usedBytes = LittleFS.usedBytes();
#endif
  info_fs.id = id;
  wsSend((uint8_t *)&info_fs, sizeof(info_fs));
  wsTask[INFO] = OFF;
  return;
}

void scanWiFi() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    scan.id = i;
    memset(scan.ssid, 0, sizeof(scan.ssid));
    WiFi.SSID(i).toCharArray(scan.ssid, 32);
    scan.channel = WiFi.channel(i);
    scan.rssi = WiFi.RSSI(i);
    scan.encryptionType = WiFi.encryptionType(i);
    // scan.isHidden = WiFi.isHidden(i);
    wsSend((uint8_t *)&scan, sizeof(scan));
  };
  wsTask[SCAN] = OFF;
}

void sendSettings() {
  wsSend((uint8_t *)&storage, sizeof(storage));
  wsTask[SETTINGS] = OFF;
}

void saveConfig() {
  EEPROM.put(CONFIG_START, storage);
  Serial.print("Save config");
  EEPROM.commit();
}

void loadConfig() {
  uint16_t version = 1678;
  EEPROM.get(CONFIG_START + 4, version);
  Serial.println(version);
  if (version == storage.version) {
    EEPROM.get(CONFIG_START, storage);
  } else {
    char idStr[8];
    sprintf(idStr, "%02X", id);
    strcat(storage.wifiSsid, idStr);
    saveConfig();
  }
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
}

void onWsEvent(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;
  if (info->final && info->index == 0 && info->len == len) {
    if (info->opcode == WS_BINARY) {
      Serial.println(data[0]);
      if (info->len == 1) {
        wsTask[data[0]] = ON;
      } else {
        if (data[0] == SETTINGS && info->len == sizeof(storage)) {
          for (size_t i = 0; i < info->len; i++) {
            *((char *)&storage + i) = data[i];
          }
          saveConfig();
        }
        if (data[0] == FILES && info->len == sizeof(files)) {
          for (size_t i = 0; i < info->len; i++) {
            *((char *)&files + i) = data[i];
          }
          wsTask[data[0]] = ON;
        }
      }
    }
  }
}

void initLittleFS() {
  if (!LittleFS.begin() && LittleFS.format()) Serial.println(F("Filesystem formatted!"));
}

void setup() {
  Serial.begin(115200);
  // WiFi.onEvent(WiFiEvent);
  EEPROM.begin(256);
  loadConfig();
  initLittleFS();
  getInfo();

  if (storage.wifiMode) {
    WiFi.mode((WiFiMode_t)storage.wifiMode);
    if (!storage.wifiDhcp) WiFi.config(storage.wifiIp, storage.wifiGeteway, storage.wifiSubnet, storage.wifiDns);
    if (storage.wifiMode == WIFI_STA) WiFi.begin(storage.wifiSsid, storage.wifiPass);
    if (storage.wifiMode == WIFI_AP) WiFi.softAP(storage.wifiSsid, storage.wifiPass);
  }
  startWebServer(storage, info_fs.totalBytes, onWsEvent);
}

void loop() {
  now = millis();
  loopWebServer(now);

  if (wsTask[SETTINGS]) sendSettings();
  if (wsTask[INFO]) getInfo();
  if (wsTask[FILES]) getFile(files.name);
  if (wsTask[REBOOT]) reboot();
  if (wsTask[SCAN]) scanWiFi();
}