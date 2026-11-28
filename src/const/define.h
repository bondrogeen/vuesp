#define CONFIG_START 0

#define CONFIG_VERSION 0

// Project defaults (network, auth, discovery, logging, device identity) live in
// vuesp.config.yaml and arrive through this generated header.
#include "../generated/vuespConfig.h"

#define DEF_WIFI_MODE WIFI_AP

#define DEF_HIDE_PASS "********"
