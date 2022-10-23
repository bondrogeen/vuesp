#ifndef webserver_H
#define webserver_H

#include "init.h"
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include "./pages/recovery.h"
#define RES_TYPE_JSON "application/json"

String status(uint8_t state);
void initServer(void (*function)(void *arg, uint8_t *data, size_t len, uint32_t wsClient));
void loopServer(uint32_t now);
void wsSend(uint8_t *message, size_t len);
// extern AsyncWebServer server;

#endif