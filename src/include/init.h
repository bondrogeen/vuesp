#ifndef INIT_H
#define INIT_H

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
void reboot();
extern Settings settings;
extern Info infoFS;

#endif