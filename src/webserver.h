#ifndef webserver_H
#define webserver_H

#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#include "./const/esp8266/settings.h"
#include "./pages/recovery.h"
#define RES_TYPE_JSON "application/json"

void startWebServer(Settings storage, uint32_t total, void (*function)(void *arg, uint8_t *data, size_t len, uint32_t wsClient));
void wsSend(uint8_t *message, size_t len);
void loopWebServer(uint32_t now);
String status(uint8_t state);
void onRecovery(AsyncWebServerRequest *request);
void onUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onReqUpdate(AsyncWebServerRequest *request);
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
void onReqUpload(AsyncWebServerRequest *request);
extern AsyncWebServer server;

#endif