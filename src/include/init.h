#ifndef INIT_H
#define INIT_H

#include <EEPROM.h>
#include <LittleFS.h>

#ifdef ESP32
// #include <AsyncTCP.h>
// #include <FS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include "../const/index.h"

void setupInit();
void saveSettings(Settings& settings);
void reboot();
uint8_t readFile(const char* path, uint8_t* buf, size_t size);
void writeFile(const char* path, const uint8_t* buf, size_t size);
extern Settings settings;
extern Info infoFS;

#endif