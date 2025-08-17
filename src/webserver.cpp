#include "./include/webserver.h"

#include "./include/device.h"
#include "./include/files.h"
#include "./include/modbus.h"
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
void wsSendAll(uint8_t *message, size_t len) {
  ws.binaryAll(message, len);
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
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    if (info->opcode == WS_BINARY) {
      if (info->final && info->index == 0 && info->len == len) {
        uint8_t task = data[0];
        if (task > KEY_END) return;
        onWsEventTasks(arg, data, len, clientID, task);
        onWsEventDevice(arg, data, len, clientID, task);
      }
    }
  }
}

void onReqUpload(AsyncWebServerRequest *request) {
  if (!request->authenticate(settings.authLogin, settings.authPass)) return request->requestAuthentication();
  uint8_t method = request->method();
  if (request->hasParam("file")) {
    const AsyncWebParameter *p = request->getParam("file");
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
    progress.size = 0;
    progress.length = request->contentLength();
    request->_tempFile = LittleFS.open(filename, "w");
    // Serial.println(filename);
  }
  if (len) request->_tempFile.write(data, len);
  if (final) request->_tempFile.close();
  sendProgress();
}

void onReqModbus(AsyncWebServerRequest *request) {
  if (!request->authenticate(settings.authLogin, settings.authPass)) return request->requestAuthentication();
  uint8_t len = 0;
  if (request->hasParam("data")) {
    const AsyncWebParameter *p = request->getParam("data");

    const char *hexStr = p->value().c_str();
    uint8_t length = strlen(hexStr) / 2;

    for (int i = 0; i < length; i++) {
      char pair[3];
      strncpy(pair, hexStr + (i * 2), 2);
      pair[2] = '\0';
      uint8_t byteValue = (uint8_t)strtol(pair, nullptr, 16);
      modbus.data[i] = byteValue;
    }

    bool isCheck = request->hasParam("crc");

    if (isCheck) {
      uint16_t crc = getCrc16(modbus.data, length);
      uint8_t bytes[2];
      uint16ToByte(crc, bytes);
      modbus.data[length] = bytes[0];
      modbus.data[length + 1] = bytes[1];
      length += 2;
    }

    len = transmitData(modbus.data, length, isCheck);
    if (!len) return request->send(422, RES_TYPE_JSON, status(0));

    AsyncResponseStream *response = request->beginResponseStream("text/html");
    for (int i = 0; i < len; i++) {
      if (modbus.data[i] < 16) {
        response->print('0');
      }
      response->print(modbus.data[i], 16);
    }
    request->send(response);
    return;
  }
  return request->send(422, RES_TYPE_JSON, status(0));
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
    progress.size = 0;
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

// void onGetData(AsyncWebServerRequest *request) {
//   if (!request->authenticate(settings.authLogin, settings.authPass)) return request->requestAuthentication();
//   AsyncResponseStream *response = request->beginResponseStream("application/x-binary", sizeof(infoFS));
//   response->write((const uint8_t *)&infoFS, sizeof(infoFS));
//   request->send(response);
// }
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
  // server.on("/get", HTTP_GET, onGetData);
  server.on("/modbus", HTTP_GET, onReqModbus);
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
