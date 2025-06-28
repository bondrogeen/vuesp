#include "./include/eeprom.h"

void initEEprom() {
  EEPROM.begin(256);
}

void saveEEprom(Settings& settings) {
  EEPROM.put(CONFIG_START, settings);
  EEPROM.commit();
}

void saveSettings(Settings& settings) {
  EEPROM.put(CONFIG_START, settings);
  EEPROM.commit();
}

void loadConfig(Settings& settings, uint32_t id) {
  uint16_t version = 345;
  EEPROM.get(CONFIG_START + 4, version);
  if (version == settings.version) {
    EEPROM.get(CONFIG_START, settings);
  } else {
    char nameDevice[20];
    sprintf(nameDevice, "%s%02X", DEF_DEVICE_NAME, id);
    strcpy(settings.wifiSsid, nameDevice);
    saveSettings(settings);
  }
}
