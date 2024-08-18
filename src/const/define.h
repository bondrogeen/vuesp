#define CONFIG_START 0

#define DEF_DEVICE_NAME "VUESP-"

// Default struct settings
#define CONFIG_VERSION 5

#define ETH_CLK_MODE ETH_CLOCK_GPIO17_OUT  // Version with not PSRAM
#define ETH_POWER_PIN -1
#define ETH_TYPE ETH_PHY_LAN8720
#define ETH_ADDR 0
#define ETH_MDC_PIN 23
#define ETH_MDIO_PIN 18

#define NRST 5

#define DEF_ETH_DHCP 1
#define DEF_ETH_IP {192, 168, 23, 135}
#define DEF_ETH_SUBNET {255, 255, 255, 0}
#define DEF_ETH_GETEWAY {192, 168, 23, 1}
#define DEF_ETH_DNS {192, 168, 23, 1}

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

#define DEF_PAHT_GPIO "/service/gpio.io"
#define DEF_PATH_SERVICE "/service"

#ifndef DEF_DEVICE_FIRMWARE
#define DEF_DEVICE_FIRMWARE {0, 0, 1}
#endif