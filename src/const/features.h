#ifndef VUESP_FEATURES_H
#define VUESP_FEATURES_H

#include "../generated/vuespConfig.h"

// Feature flags come from vuesp.config.yaml through the generated header.
// A build flag such as -DVUESP_MODBUS=1 can still override the generated value.
//
// Sources of disabled features remain in the repository; the feature is either
// excluded from the build or guarded by #if VUESP_*.

// Explicit permission to enable Modbus on ESP8266 (for debug builds without
// the web UI). Modbus needs about 21 KB of static RAM.
#ifndef VUESP_MODBUS_ALLOW_ESP8266
#define VUESP_MODBUS_ALLOW_ESP8266 0
#endif

#if VUESP_MODBUS && defined(ESP8266) && !VUESP_MODBUS_ALLOW_ESP8266
#error "VUESP_MODBUS needs about 21 KB of RAM and breaks the ESP8266 web UI (OOM). Use an ESP32 board or set -D VUESP_MODBUS_ALLOW_ESP8266=1 explicitly."
#endif

#endif
