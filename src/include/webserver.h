#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>

#include "../pages/recovery.h"
#include "./init.h"
#define RES_TYPE_JSON "application/json"

String status(uint8_t state);
void setupServer();
void loopServer(uint32_t now);
void wsSend(uint8_t *message, size_t len);

#endif