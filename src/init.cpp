#include "./include/init.h"

WiFiUDP udp;

Info infoFS = {KEY_INFO, DEF_DEVICE_FIRMWARE, 0, 0, 0};

FindUDP udpPacket = {
    0,
};

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

uint32_t lastTimeInit = 0;
uint8_t isConnected = 0;

void reboot() {
  ESP.restart();
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  if (event == 7 || event == 3) {
    Serial.println(WiFi.localIP());
    isConnected = 1;
  }
}

void initWiFi() {
  if (settings.wifiMode) {
    WiFi.setHostname(settings.wifiSsid);
    WiFi.mode((WiFiMode_t)settings.wifiMode);
    if (!settings.wifiDhcp) WiFi.config(settings.wifiIp, settings.wifiGeteway, settings.wifiSubnet, settings.wifiDns);
    if (settings.wifiMode == WIFI_STA) WiFi.begin(settings.wifiSsid, settings.wifiPass);
    if (settings.wifiMode == WIFI_AP) WiFi.softAP(settings.wifiSsid, settings.wifiPass);
    WiFi.onEvent(WiFiEvent);
    udp.begin(UDP_PORT);
  }
}

void udpSend(uint8_t *message, size_t len) {
  udp.beginPacket(UDP_IP, UDP_PORT);
  udp.write(message, len);
  udp.endPacket();
}

void udpIncoming() {
  int size = udp.parsePacket();
  if (!size) return;
  udp.read((uint8_t *)&udpPacket, sizeof(udpPacket));
  if (udpPacket.command == 255) {
    udpPacket.id = infoFS.id;
    udpPacket.command = 254;
    udpSend((uint8_t *)&udpPacket, sizeof(udpPacket));
  }
}

void loopWiFi(uint32_t now) {
  if (now - lastTimeInit > 10000) {
    lastTimeInit = now;
  }
  if (isConnected) {
    udpIncoming();
  }
}
