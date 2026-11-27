#ifndef VUESP_FEATURES_H
#define VUESP_FEATURES_H

// Флаги необязательных возможностей шаблона.
//
// Исходники выключенной возможности остаются в репозитории; из сборки она
// просто исключается, поэтому не занимает ни flash, ни RAM. Включать лучше
// построчно для нужного env:
//
//   build_flags = -D VUESP_MODBUS=1

// Modbus RTU (src/modbus). Профиль, входящий профиль и буферы приёма занимают
// около 21 КБ статической RAM: на ESP8266 это больше половины свободного heap
// и web UI падает с OOM. Поэтому по умолчанию возможность не компилируется.
#ifndef VUESP_MODBUS
#define VUESP_MODBUS 0
#endif

// Осознанное разрешение Modbus на ESP8266 (например, для отладки без web UI).
#ifndef VUESP_MODBUS_ALLOW_ESP8266
#define VUESP_MODBUS_ALLOW_ESP8266 0
#endif

#if VUESP_MODBUS && defined(ESP8266) && !VUESP_MODBUS_ALLOW_ESP8266
#error "VUESP_MODBUS needs about 21 KB of RAM and breaks the ESP8266 web UI (OOM). Use an ESP32 board or set -D VUESP_MODBUS_ALLOW_ESP8266=1 explicitly."
#endif

#endif
