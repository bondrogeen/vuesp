#include "init.h"

#if defined(ESP8266)
uint32_t id = ESP.getChipId();
#elif defined(ESP32)
uint32_t id = (uint32_t)(ESP.getEfuseMac() >> 32);
#endif

// WiFiClient WiFIclient;

Info infoFS = {INFO, DEF_DEVICE_FIRMWARE, 0, 0, 0};

Settings settings = {
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

void reboot() {
  ESP.restart();
}

void saveSettings(Settings &settings) {
  EEPROM.put(CONFIG_START, settings);
  EEPROM.commit();
}

void loadConfig(Settings &settings) {
  uint16_t version = 1678;
  EEPROM.get(CONFIG_START + 4, version);
  if (version == settings.version) {
    EEPROM.get(CONFIG_START, settings);
  } else {
    char idStr[8];
    sprintf(idStr, "%02X", id);
    strcat(settings.wifiSsid, idStr);
    saveSettings(settings);
  }
}

void getInfo() {
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
  if (!LittleFS.begin() && LittleFS.format()) Serial.println(F("Filesystem formatted!"));
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
}

void initWiFi() {
  if (settings.wifiMode) {
    WiFi.mode((WiFiMode_t)settings.wifiMode);
    if (!settings.wifiDhcp) WiFi.config(settings.wifiIp, settings.wifiGeteway, settings.wifiSubnet, settings.wifiDns);
    if (settings.wifiMode == WIFI_STA) WiFi.begin(settings.wifiSsid, settings.wifiPass);
    if (settings.wifiMode == WIFI_AP) WiFi.softAP(settings.wifiSsid, settings.wifiPass);
  }
  WiFi.onEvent(WiFiEvent);
}

void initApp() {
  initFS();
  EEPROM.begin(256);
  loadConfig(settings);
  getInfo();
  initWiFi();

  Serial.println(CURRENT_TIME);

  Serial.println("event");
  Serial.println(settings.wifiMode);
  Serial.println(settings.wifiSsid);
  Serial.println(infoFS.totalBytes);
  Serial.println(infoFS.id);

}