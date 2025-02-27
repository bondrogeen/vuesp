#ifndef INIT_H
#define INIT_H

#define UDP_PORT 12345
#define UDP_IP {255, 255, 255, 255}

#include <WiFiUdp.h>

#ifdef ESP32
// #include <AsyncTCP.h>
// #include <FS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include "../const/index.h"

void initWiFi();
void loopWiFi(uint32_t now);
void reboot();
extern Settings settings;
extern Info infoFS;

#endif