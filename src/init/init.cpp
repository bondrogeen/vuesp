#include "./init.h"

#if defined(ARDUINO_ESP32C3_DEV)
#include "esp_wifi.h"
#endif

uint8_t isFirstWifi = 0;

Info infoFS = {KEY_INFO, DEF_DEVICE_FIRMWARE, 0, 0, 0};
Event event = {KEY_EVENT, 0, 0, 0, 0, ""};

Settings settings = {
    KEY_SETTINGS,
    DEF_WIFI_DHCP,
    DEF_WIFI_MODE,
    DEF_AUTH_MODE,
    CONFIG_VERSION,
    0,
    DEF_DISCOVERY,
    DEF_DISCOVERY_INTERVAL,
    DEF_DISCOVERY_PORT,
    DEF_WIFI_IP,
    DEF_WIFI_SUBNET,
    DEF_WIFI_GETEWAY,
    DEF_WIFI_DNS,
    DEF_WIFI_SSID,
    DEF_WIFI_PASS,
    DEF_AUTH_LOGIN,
    DEF_AUTH_PASS};

uint32_t lastTimeInit = 0;
uint32_t lastWifiAttempt = 0;
uint8_t wifiAttempts = 0;
uint8_t isConnected = 0;

void reboot() {
  ESP.restart();
}

void WiFiEvent(WiFiEvent_t event) {
#if VUESP_LOG_LEVEL >= 2
  Serial.printf("[WiFi-event] event: %d\n", event);
#endif
  if (event == 7 || event == 3 || event == 115) {
#if VUESP_LOG_LEVEL >= 1
    Serial.println(WiFi.localIP());
#endif
    isConnected = 1;
  }
  if (event == 8 || event == 5 || event == 116) {
#if VUESP_LOG_LEVEL >= 1
    Serial.println("WiFi disconnected");
#endif
    isConnected = 0;
  }
}

void initWiFi() {
  if (settings.wifiMode) {
#if defined(ARDUINO_ESP32C3_DEV)
    esp_wifi_set_max_tx_power(WIFI_POWER_8_5dBm);
#endif
    WiFi.onEvent(WiFiEvent);
    WiFi.setHostname(settings.wifiSsid);
    if (settings.wifiMode == WIFI_STA)
      WiFi.mode(WIFI_STA);
    else if (settings.wifiMode == WIFI_AP)
      WiFi.mode(WIFI_AP);
    else
      WiFi.mode(WIFI_AP_STA);

    WiFi.setSleep(false);

    if (!settings.wifiDhcp) {
      WiFi.config(settings.wifiIp, settings.wifiGateway, settings.wifiSubnet, settings.wifiDns);
    }
    if (settings.wifiMode == WIFI_STA) WiFi.begin(settings.wifiSsid, settings.wifiPass);
    if (settings.wifiMode == WIFI_AP) WiFi.softAP(settings.wifiSsid, settings.wifiPass);
  }
}

void copyIp(uint8_t saveIp[4], IPAddress ip) {
  for (size_t i = 0; i < 4; i++) {
    saveIp[i] = ip[i];
  }
}

void updateIp() {
  if (!isFirstWifi && settings.wifiDhcp) {
    isFirstWifi = 1;
    copyIp(settings.wifiIp, WiFi.localIP());
    copyIp(settings.wifiGateway, WiFi.gatewayIP());
    copyIp(settings.wifiSubnet, WiFi.subnetMask());
    copyIp(settings.wifiDns, WiFi.dnsIP());
  }
}

void loopWiFi(uint32_t now) {
  if (now - lastTimeInit > 10000) {
    lastTimeInit = now;
  }
  if (isConnected || WiFi.status() == WL_CONNECTED) {
    updateIp();
    wifiAttempts = 0;
    return;
  }

  if (settings.wifiMode != WIFI_STA || !settings.wifiSsid[0]) return;
  if (now - lastWifiAttempt < 15000) return;

  lastWifiAttempt = now;
  if (WiFi.status() == WL_CONNECTED) {
    wifiAttempts = 0;
    return;
  }

  wifiAttempts++;
  if (wifiAttempts <= 3) {
    WiFi.reconnect();
  } else {
    WiFi.begin(settings.wifiSsid, settings.wifiPass);
    if (wifiAttempts >= 12) wifiAttempts = 1;
  }
}
