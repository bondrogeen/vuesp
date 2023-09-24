#include "./include/webserver.h"

#include "./include/device.h"
#include "./include/tasks.h"

AsyncWebServer server(80);
AsyncWebSocket ws("/esp");
// AsyncWebSocketClient *client;

Ping ping = {KEY_PING};
Progress progress = {KEY_PROGRESS, 5, 0, 0, 0};

uint32_t countClient = 0;
uint32_t clientID = 0;
uint8_t hold = 255;
uint32_t lastTime = 0;

String status(uint8_t state) {
  return (state) ? "{\"state\":true}" : "{\"state\":false}";
}

void wsSend(uint8_t *message, size_t len) {
  ws.binary(clientID, message, len);
}

void sendProgress() {
  if (progress.status == 1 || progress.status == 0 || hold > 15) {
    ws.binaryAll((uint8_t *)&progress, sizeof(progress));
    hold = 0;
  }
  hold++;
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  clientID = client->id();
  if (type == WS_EVT_CONNECT)
    countClient += clientID;
  else if (type == WS_EVT_DISCONNECT)
    countClient -= clientID;
  else if (type == WS_EVT_DATA) {
    onWsEventTasks(arg, data, len, clientID);
    onWsEventDevice(arg, data, len, clientID);
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
  if (isReboot) reboot();
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
    if (!Update.begin((cmd == 100) ? infoFS.totalBytes : maxSketchSpace, cmd)) {
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

void onRedirectRecovery(AsyncWebServerRequest *request) {
  request->redirect("/recovery");
}

void onRedirectHome(AsyncWebServerRequest *request) {
  request->redirect("/");
}

void setupServer() {
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
  server.on("/", HTTP_GET, onRedirectRecovery);
  server.on("*", HTTP_ANY, onRedirectHome);
  server.begin();
}

void loopServer(uint32_t now) {
  if (countClient) {
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
