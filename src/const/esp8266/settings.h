#include "./const/base.h"

struct Settings {
  uint8_t init;
  uint8_t wifiDhcp;
  uint8_t wifiMode;
  uint8_t authMode;
  uint16_t version;
  uint16_t device;

  uint8_t wifiIp[4];
  uint8_t wifiSubnet[4];
  uint8_t wifiGeteway[4];
  uint8_t wifiDns[4];

  char wifiSsid[32];
  char wifiPass[32];
  char authLogin[12];
  char authPass[12];
};