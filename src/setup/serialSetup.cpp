#include "./serialSetup.h"

#if defined(ESP8266)
#include <ESP8266WiFi.h>
#elif defined(ESP32)
#include <WiFi.h>
#endif

#include <string.h>

#include "../eeprom/eeprom.h"
#include "../init/init.h"
#include "../utils/text.h"

static char serialLine[201];
static uint16_t serialLen = 0;

static void printWifiScan() {
  Serial.println(F("@VUESP:SCAN:BEGIN"));

  auto oldMode = WiFi.getMode();
  WiFi.mode(WIFI_AP_STA);

  int count = WiFi.scanNetworks();
  if (count < 0) {
    Serial.println(F("@VUESP:SCAN:ERROR"));
  } else {
    for (int i = 0; i < count; i++) {
      char ssid[33];
      WiFi.SSID(i).toCharArray(ssid, sizeof(ssid));
      textSanitizeSsid(ssid);
      int secure = WiFi.encryptionType(i) != 0 ? 1 : 0;
      Serial.printf("@VUESP:WIFI:%s\t%d\t%d\n", ssid, WiFi.RSSI(i), secure);
    }
  }

  WiFi.scanDelete();
  WiFi.mode(oldMode);
  Serial.println(F("@VUESP:SCAN:END"));
}

static void setWifi(const char* command) {
  const char* first = strchr(command, '\t');
  if (!first) {
    Serial.println(F("@VUESP:SET:ERROR"));
    return;
  }

  const char* second = strchr(first + 1, '\t');
  const char* commandEnd = command + strlen(command);

  char ssid[32];
  char pass[32];
  if (!textCopyTrimmed(ssid, sizeof(ssid), first + 1, second ? second : commandEnd) ||
      !textCopyRaw(pass, sizeof(pass), second ? second + 1 : commandEnd, commandEnd) || !ssid[0]) {
    Serial.println(F("@VUESP:SET:ERROR"));
    return;
  }

  memset(settings.wifiSsid, 0, sizeof(settings.wifiSsid));
  memset(settings.wifiPass, 0, sizeof(settings.wifiPass));
  strncpy(settings.wifiSsid, ssid, sizeof(settings.wifiSsid) - 1);
  strncpy(settings.wifiPass, pass, sizeof(settings.wifiPass) - 1);
  settings.wifiMode = WIFI_STA;
  settings.wifiDhcp = 1;

  if (!saveSettings(settings)) {
    Serial.println(F("@VUESP:SET:ERROR"));
    return;
  }

  Serial.println(F("@VUESP:SET:OK"));
  delay(250);
  reboot();
}

static void handleSerialLine(char* command) {
  textTrim(command);
  if (strncmp(command, "VUESP:", 6) != 0) return;

  if (strcmp(command, "VUESP:PING") == 0) {
    Serial.println(F("@VUESP:PONG"));
  } else if (strcmp(command, "VUESP:SCAN") == 0) {
    printWifiScan();
  } else if (strncmp(command, "VUESP:SET\t", 10) == 0) {
    setWifi(command);
  }
}

void loopSerialSetup() {
  while (Serial.available() > 0) {
    char value = static_cast<char>(Serial.read());
    if (value == '\n' || value == '\r') {
      if (serialLen) {
        serialLine[serialLen] = '\0';
        handleSerialLine(serialLine);
        serialLen = 0;
      }
    } else if (serialLen < sizeof(serialLine) - 1) {
      serialLine[serialLen++] = value;
    }
  }
}
