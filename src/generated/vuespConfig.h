#ifndef VUESP_GENERATED_CONFIG_H
#define VUESP_GENERATED_CONFIG_H

// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

#define VUESP_CONFIG_VERSION 1
#define VUESP_PROJECT_NAME "vuesp."
#define VUESP_BOARD 1
#define VUESP_FIRMWARE_MAJOR 5
#define VUESP_FIRMWARE_MINOR 0
#define VUESP_FIRMWARE_PATCH 13

// Features. Build flags may override these values.
#ifndef VUESP_SCRIPT
#define VUESP_SCRIPT 1
#endif
#ifndef VUESP_MODBUS
#define VUESP_MODBUS 0
#endif
#ifndef VUESP_SERIAL
#define VUESP_SERIAL 1
#endif
#ifndef VUESP_DISCOVERY
#define VUESP_DISCOVERY 1
#endif
#ifndef VUESP_DASHBOARD
#define VUESP_DASHBOARD 1
#endif

// Logging and network defaults.
#ifndef VUESP_LOG_LEVEL
#define VUESP_LOG_LEVEL 3
#endif
#ifndef DEF_WIFI_DHCP
#define DEF_WIFI_DHCP 1
#endif
#ifndef DEF_WIFI_IP
#define DEF_WIFI_IP {192, 168, 1, 12}
#endif
#ifndef DEF_WIFI_SUBNET
#define DEF_WIFI_SUBNET {255, 255, 255, 0}
#endif
#ifndef DEF_WIFI_GETEWAY
#define DEF_WIFI_GETEWAY {192, 168, 1, 1}
#endif
#ifndef DEF_WIFI_DNS
#define DEF_WIFI_DNS {8, 8, 8, 8}
#endif
#ifndef DEF_WIFI_SSID
#define DEF_WIFI_SSID ""
#endif
#ifndef DEF_WIFI_PASS
#define DEF_WIFI_PASS ""
#endif
#ifndef DEF_AUTH_MODE
#define DEF_AUTH_MODE 1
#endif
#ifndef DEF_AUTH_LOGIN
#define DEF_AUTH_LOGIN "admin"
#endif
#ifndef DEF_AUTH_PASS
#define DEF_AUTH_PASS "admin"
#endif
#ifndef DEF_DISCOVERY
#define DEF_DISCOVERY 1
#endif
#ifndef DEF_DISCOVERY_INTERVAL
#define DEF_DISCOVERY_INTERVAL 5
#endif
#ifndef DEF_DISCOVERY_PORT
#define DEF_DISCOVERY_PORT 12345
#endif
#ifndef DEF_DEVICE_NAME
#define DEF_DEVICE_NAME "vuesp."
#endif
#ifndef DEF_DEVICE_FIRMWARE
#define DEF_DEVICE_FIRMWARE {5, 0, 13}
#endif
#ifndef DEF_BOARD
#define DEF_BOARD 1
#endif

// Script engine limits.
#ifndef VBC_MAX_CODE_BYTES
#define VBC_MAX_CODE_BYTES 512
#endif
#ifndef VBC_CACHE_SLOTS
#define VBC_CACHE_SLOTS 6
#endif
#ifndef VBC_MAX_SCRIPTS
#define VBC_MAX_SCRIPTS 15
#endif
#ifndef VBC_MAX_SAVED_SCRIPTS
#define VBC_MAX_SAVED_SCRIPTS 15
#endif
#ifndef VBC_MAX_CONTEXTS
#define VBC_MAX_CONTEXTS 15
#endif
#ifndef VBC_STACK_SIZE
#define VBC_STACK_SIZE 64
#endif
#ifndef VBC_SCRATCH_BYTES
#define VBC_SCRATCH_BYTES 256
#endif
#ifndef VBC_MAX_CALL_DEPTH
#define VBC_MAX_CALL_DEPTH 4
#endif
#ifndef MAX_UINT_VARS
#define MAX_UINT_VARS 10
#endif
#ifndef MAX_INT_VARS
#define MAX_INT_VARS 10
#endif
#ifndef MAX_FLOAT_VARS
#define MAX_FLOAT_VARS 5
#endif
#ifndef MAX_STRING_VARS
#define MAX_STRING_VARS 2
#endif
#ifndef MAX_ARRAY_VARS
#define MAX_ARRAY_VARS 5
#endif
#ifndef MAX_STRING_LEN
#define MAX_STRING_LEN 32
#endif
#ifndef MAX_ARRAY_SIZE
#define MAX_ARRAY_SIZE 64
#endif
#ifndef MAX_PWM_VALUE
#define MAX_PWM_VALUE 255
#endif
#ifndef MAX_EXTERNAL_FUNCTIONS
#define MAX_EXTERNAL_FUNCTIONS 16
#endif
#ifndef MAX_FUNCTION_PARAMS
#define MAX_FUNCTION_PARAMS 5
#endif
#ifndef MAX_CALL_PARAMS
#define MAX_CALL_PARAMS 4
#endif
#ifndef MAX_EVENT_PARAMS
#define MAX_EVENT_PARAMS 4
#endif

#endif
