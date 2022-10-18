#include "webserver.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/esp");
// AsyncWebSocketClient *client;
Settings settings;

Ping ping = {PING};
Progress progress = {PROGRESS, 5, 0, 0, 0};

uint32_t wsClient = 0;
uint8_t test = 255;
uint8_t wsConnected = false;
uint32_t lastTime = 0;
uint32_t totalBytes;

String status(uint8_t state) {
  return (state) ? "{\"state\":true}" : "{\"state\":false}";
}

void wsSend(uint8_t *message, size_t len) {
  ws.binary(wsClient, message, len);
}

void sendProgress() {
  if (progress.status == 1 || progress.status == 0 || test > 15) {
    Serial.print("Save config");
    ws.binaryAll((uint8_t *)&progress, sizeof(progress));
    test = 0;
  }
  test++;
}

void (*p_function)(void *arg, uint8_t *data, size_t len, uint32_t clientId);

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  wsClient = client->id();
  if (type == WS_EVT_CONNECT)
    wsConnected = true;
  else if (type == WS_EVT_DISCONNECT)
    wsConnected = false;
  else if (type == WS_EVT_DATA) {
    if (p_function != NULL) {
      (*p_function)(arg, data, len, wsClient);
    }
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->final && info->index == 0 && info->len == len) {
      if (info->opcode == WS_BINARY) {
        Serial.println(data[0]);
      }
    }
  }
}

void onReqUpload(AsyncWebServerRequest *request) {
  if (!request->authenticate(settings.authLogin, settings.authPass)) return request->requestAuthentication();
  uint8_t method = request->method();
  if (request->hasParam("file")) {
    AsyncWebParameter *p = request->getParam("file");
    if (method == HTTP_GET)
      if (LittleFS.exists(p->value())) return request->send(LittleFS, p->value(), String(), true);
    if (method == HTTP_DELETE)
      if (LittleFS.exists(p->value()) && (LittleFS.remove(p->value()) || LittleFS.rmdir(p->value()))) return request->send(200, RES_TYPE_JSON, status(1));
  }
  if (method == HTTP_POST && request->hasParam("format"))
    if (LittleFS.format()) return request->send(200, RES_TYPE_JSON, status(1));

  if (method == HTTP_POST) {
    progress.size = 0;
    progress.status = 0;
    return request->send(200, RES_TYPE_JSON, status(1));
  }
  request->send(404, RES_TYPE_JSON, status(0));
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  progress.size += len;
  progress.status = !index ? 1 : 2;
  if (!index) {
    progress.length = request->contentLength();
    request->_tempFile = LittleFS.open(filename, "w");
  }
  if (len) request->_tempFile.write(data, len);
  if (final) request->_tempFile.close();
  sendProgress();
}

void onReqUpdate(AsyncWebServerRequest *request) {
  uint8_t isReboot = !Update.hasError();
  AsyncWebServerResponse *response = request->beginResponse(200, RES_TYPE_JSON, status(isReboot));
  response->addHeader("Connection", "close");
  request->send(response);
  if (isReboot) ESP.restart();
}

void onUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  progress.size += len;
  progress.status = !index ? 1 : 2;
  if (!index) {
    progress.length = request->contentLength();
#if defined(ESP8266)
    Update.runAsync(true);
#endif
    int cmd = (filename == "littlefs.bin") ? 100 : U_FLASH;
    size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin((cmd == 100) ? totalBytes : maxSketchSpace, cmd)) {
      Update.printError(Serial);
      return request->send(400, RES_TYPE_JSON, status(0));
    }
  }
  if (!Update.hasError() && Update.write(data, len) != len) Update.printError(Serial);
  if (final && !Update.end(true)) Update.printError(Serial);
  sendProgress();
}

void onRecovery(AsyncWebServerRequest *request) {
  if (!request->authenticate(settings.authLogin, settings.authPass)) return request->requestAuthentication();
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", recovery, sizeof(recovery));
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void startWebServer(Settings storage, uint32_t total, void (*function)(void *arg, uint8_t *data, size_t len, uint32_t clientId)) {
  p_function = function;
  totalBytes = total;
  settings = storage;
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
  if (settings.authMode) {
    server.serveStatic("/", LittleFS, "/www/").setCacheControl("max-age=600").setDefaultFile("index.html").setAuthentication(settings.authLogin, settings.authPass);
  } else {
    server.serveStatic("/", LittleFS, "/www/").setCacheControl("max-age=600").setDefaultFile("index.html");
  }

  server.on("/fs", HTTP_ANY, onReqUpload, onUpload);
  server.on("/update", HTTP_POST, onReqUpdate, onUpdate);
  server.on("/recovery", HTTP_GET, onRecovery);
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) { request->redirect("/recovery"); });
  server.on("*", HTTP_ANY, [](AsyncWebServerRequest *request) { request->redirect("/"); });
  server.begin();
}

void loopWebServer(uint32_t now) {
  if (wsConnected) {
    if (now - lastTime > 1000) {
      lastTime = now;
      if (progress.status == 0) {
        sendProgress();
        progress.status = 5;
      } else {
        ws.binaryAll((uint8_t *)&ping, sizeof(ping));
      }
      // ws.cleanupClients();
    }
  }
}
