#include "./include/init.h"

uint8_t isFirstWifi = 0;

WiFiUDP udp;

Info infoFS = {KEY_INFO, DEF_DEVICE_FIRMWARE, 0, 0, 0};

struct FindUDP {
  uint8_t command;
  uint8_t firmware[3];
  uint32_t deviceId;
  char name[20];
} udpPacket = {
    99,
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
  if (event == 7 || event == 3, event == 115) {
    Serial.println(WiFi.localIP());
    isConnected = 1;
  }
}

void initWiFi() {
  if (settings.wifiMode) {
    WiFi.setHostname(settings.wifiSsid);
    WiFi.mode((WiFiMode_t)settings.wifiMode);
    if (!settings.wifiDhcp) {
      WiFi.config(settings.wifiIp, settings.wifiGateway, settings.wifiSubnet, settings.wifiDns);
    }
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

void udpIncoming() {
  int size = udp.parsePacket();
  if (!size) return;
  udp.read((uint8_t *)&udpPacket, sizeof(udpPacket));
  if (udpPacket.command == 99) {
    Serial.println(udpPacket.deviceId);
    udpPacket.deviceId = infoFS.id;
    udpPacket.firmware[0] = infoFS.firmware[0];
    udpPacket.firmware[1] = infoFS.firmware[1];
    udpPacket.firmware[2] = infoFS.firmware[2];
    strcpy(udpPacket.name, infoFS.name);
    udpPacket.command = 98;
    udpSend((uint8_t *)&udpPacket, sizeof(udpPacket));
  }
}

void loopWiFi(uint32_t now) {
  if (now - lastTimeInit > 10000) {
    lastTimeInit = now;
  }
  if (isConnected) {
    udpIncoming();
    updateIp();
  }
}
