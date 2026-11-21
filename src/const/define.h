#define CONFIG_START 0

#define CONFIG_VERSION 0

// Logging: 0 — silent, 1 — errors/boot, 2 — events and script lifecycle,
// 3 — per-port and per-data debug (hot path).
// Local builds default to full logging; CI passes -DVUESP_LOG_LEVEL=0.
#ifndef VUESP_LOG_LEVEL
#define VUESP_LOG_LEVEL 3
#endif

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP \
  {192, 168, 1, 12}
#define DEF_WIFI_SUBNET \
  {255, 255, 255, 0}
#define DEF_WIFI_GETEWAY \
  {192, 168, 1, 1}
#define DEF_WIFI_DNS \
  {8, 8, 8, 8}

#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID ""
#define DEF_WIFI_PASS ""

#define DEF_AUTH_MODE 1
#define DEF_AUTH_LOGIN "admin"
#define DEF_AUTH_PASS "admin"

#define DEF_DISCOVERY 1
#define DEF_DISCOVERY_INTERVAL 5
#define DEF_DISCOVERY_PORT 12345

#define DEF_HIDE_PASS "********"

#ifndef DEF_DEVICE_NAME
#define DEF_DEVICE_NAME "VUESP-"
#endif

#ifndef DEF_DEVICE_FIRMWARE
#define DEF_DEVICE_FIRMWARE {0, 0, 1}
#endif
#ifndef DEF_BOARD
#define DEF_BOARD 1
#endif