#define CONFIG_START 0

// Default struct settings
#define CONFIG_VERSION 4

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP \
  { 192, 168, 1, 12 }
#define DEF_WIFI_SUBNET \
  { 255, 255, 255, 0 }
#define DEF_WIFI_GETEWAY \
  { 192, 168, 1, 1 }
#define DEF_WIFI_DNS \
  { 8, 8, 8, 8 }

#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID "VUESP-"
#define DEF_WIFI_PASS ""

#define DEF_AUTH_MODE 1
#define DEF_AUTH_LOGIN "admin"
#define DEF_AUTH_PASS "admin"

#define DEF_DEVICE_FIRMWARE \
  { 0, 0, 6 }