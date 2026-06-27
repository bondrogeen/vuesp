#ifndef EEPROM_H
#define EEPROM_H

#include <EEPROM.h>
#include <stdio.h>

#include "../const/index.h"

#define SETTINGS_VERSION 12345

void initEEprom();
bool saveSettings(Settings& settings);
void loadConfig(Settings& settings, uint32_t id);

#endif