#ifndef EEPROM_H
#define EEPROM_H

#include <EEPROM.h>
#include<stdio.h>

#include "../const/index.h"

void initEEprom();
void saveSettings(Settings& settings);
void loadConfig(Settings& settings, uint32_t id);

#endif