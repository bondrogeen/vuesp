#include <ArduinoOTA.h>
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

#ifdef ESP32
#include <AsyncTCP.h>
#include <FS.h>
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif

#include "./pages/recovery.h"

#if defined(ESP8266)
uint32_t id = ESP.getChipId();
#include "./const/esp8266/settings.h"
#elif defined(ESP32)
#include "./const/esp32/settings.h"
uint32_t id = = ESP.getEfuseMac();
#endif

WiFiClient WiFIclient;
AsyncWebServer server(80);
AsyncWebSocket ws("/esp");
// AsyncWebSocketClient *client;

uint8_t wsClient = 0;
uint8_t wsTask[END];
uint8_t wsConnected = false;

uint32_t now;
uint32_t lastTime = 0;
FSInfo fs_info;

String status(uint8_t state) {
  return (state) ? "{\"state\":true}" : "{\"state\":false}";
}

void getFile(char *name) {
  Dir dir = LittleFS.openDir(name);
  while (dir.next()) {
    memset(files.name, 0, sizeof(files.name));
    dir.fileName().toCharArray(files.name, 32);
    files.size = dir.fileSize();
    files.isFile = dir.isFile();
    files.isDir = dir.isDirectory();
    ws.binary(wsClient, (uint8_t *)&files, sizeof(files));
  }
  wsTask[FILES] = OFF;
}

void reboot() {
  ESP.restart();
}

void sendProgress() {
  ws.binary(wsClient, (uint8_t *)&progress, sizeof(progress));
}

void getInfo() {
  LittleFS.info(fs_info);
  info_fs.totalBytes = fs_info.totalBytes;
  info_fs.usedBytes = fs_info.usedBytes;
  info_fs.id = id;
  ws.binary(wsClient, (uint8_t *)&info_fs, sizeof(info_fs));
  wsTask[INFO] = OFF;
  return;
}

void scanWiFi() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    scan.id = i;
    memset(scan.ssid, 0, sizeof(scan.ssid));
    WiFi.SSID(i).toCharArray(scan.ssid, 32);
    scan.channel = WiFi.channel(i);
    scan.rssi = WiFi.RSSI(i);
    scan.encryptionType = WiFi.encryptionType(i);
    scan.isHidden = WiFi.isHidden(i);
    ws.binary(wsClient, (uint8_t *)&scan, sizeof(scan));
  };
  wsTask[SCAN] = OFF;
}

void sendSettings() {
  ws.binary(wsClient, (uint8_t *)&storage, sizeof(storage));
  wsTask[SETTINGS] = OFF;
}

void saveConfig() {
  EEPROM.put(CONFIG_START, storage);
  Serial.print("Save config");
  EEPROM.commit();
}

void loadConfig() {
  uint16_t version = 1678;
  EEPROM.get(CONFIG_START + 4, version);
  Serial.println(version);
  if (version == storage.version) {
    EEPROM.get(CONFIG_START, storage);
  } else {
    char idStr[8];
    sprintf(idStr, "%02X", id);
    strcat(storage.wifiSsid, idStr);
    saveConfig();
  }
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
  if (type == WS_EVT_CONNECT) {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    wsConnected = true;
    wsClient = client->id();
  } else if (type == WS_EVT_DISCONNECT) {
    wsConnected = false;
  } else if (type == WS_EVT_ERROR) {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  } else if (type == WS_EVT_PONG) {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  } else if (type == WS_EVT_DATA) {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len) {
      wsClient = client->id();
      if (info->opcode == WS_TEXT) {
        Serial.println("WS_TEXT");
        for (size_t i = 0; i < info->len; i++) {
          msg += (char)data[i];
        }
        Serial.println(msg.c_str());
      } else {
        Serial.println("WS_BINAR");
        Serial.println(data[0]);

        if (info->len == 1) {
          wsTask[data[0]] = ON;
        } else {
          if (data[0] == SETTINGS && info->len == sizeof(storage)) {
            for (size_t i = 0; i < info->len; i++) {
              *((char *)&storage + i) = data[i];
            }
            saveConfig();
          }
          if (data[0] == FILES && info->len == sizeof(files)) {
            for (size_t i = 0; i < info->len; i++) {
              *((char *)&files + i) = data[i];
            }
            wsTask[data[0]] = ON;
          }
        }
      }
    } else {
      if (info->index == 0) {
        if (info->num == 0) Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT) {
        for (size_t i = 0; i < len; i++) {
          msg += (char)data[i];
        }
      } else {
        char buff[3];
        for (size_t i = 0; i < len; i++) {
          sprintf(buff, "%02x", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len) {
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final) {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
}

void onReqUpload(AsyncWebServerRequest *request) {
  if (!request->authenticate(storage.authLogin, storage.authPass)) return request->requestAuthentication();
  uint8_t method = request->method();
  if (request->hasParam("file")) {
    AsyncWebParameter *p = request->getParam("file");
    if (method == HTTP_GET)
      if (LittleFS.exists(p->value())) return request->send(LittleFS, p->value(), String(), true);
    if (method == HTTP_DELETE)
      if (LittleFS.exists(p->value()) && (LittleFS.remove(p->value()) || LittleFS.rmdir(p->value()))) return request->send(200, RES_TYPE_JSON, status(1));
    ;
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
  if (isReboot) wsTask[REBOOT] = ON;
  request->send(200, RES_TYPE_JSON, status(isReboot));
}

void onUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  progress.size += len;
  progress.status = !index ? 1 : 2;

  if (!index) {
    progress.length = request->contentLength();
    Update.runAsync(true);
    int cmd = (filename == "littlefs.bin") ? U_FS : U_FLASH;
    size_t fsSize = ((size_t)&_FS_end - (size_t)&_FS_start);
    size_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin((cmd == U_FS) ? fsSize : maxSketchSpace, cmd)) {
      Update.printError(Serial);
      return request->send(400, RES_TYPE_JSON, status(0));
    }
  }
  if (!Update.hasError() && Update.write(data, len) != len) Update.printError(Serial);
  if (final && !Update.end(true)) Update.printError(Serial);
  sendProgress();
}

void onRecovery(AsyncWebServerRequest *request) {
  if (!request->authenticate(storage.authLogin, storage.authPass)) return request->requestAuthentication();
  AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", recovery, sizeof(recovery));
  response->addHeader("Content-Encoding", "gzip");
  request->send(response);
}

void initLittleFS() {
  if (!LittleFS.begin() && LittleFS.format()) {
    Serial.println(F("Filesystem formatted!"));
  }
}

void setup() {
  Serial.begin(115200);
  // WiFi.onEvent(WiFiEvent);
  EEPROM.begin(256);
  loadConfig();
  initLittleFS();

  if (storage.wifiMode) {
    if (!storage.wifiDhcp) {
      WiFi.config(storage.wifiIp, storage.wifiGeteway, storage.wifiSubnet, storage.wifiDns);
    }
    WiFi.mode((WiFiMode_t)storage.wifiMode);
    if (storage.wifiMode == WIFI_STA) WiFi.begin(storage.wifiSsid, storage.wifiPass);
    if (storage.wifiMode == WIFI_AP) WiFi.softAP(storage.wifiSsid, storage.wifiPass);
  }

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  if (storage.authMode) {
    server.serveStatic("/", LittleFS, "/www/").setCacheControl("max-age=600").setDefaultFile("index.html").setAuthentication(storage.authLogin, storage.authPass);
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

void loop() {
  now = millis();
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
  if (wsTask[SETTINGS]) sendSettings();
  if (wsTask[INFO]) getInfo();
  if (wsTask[FILES]) getFile(files.name);
  if (wsTask[REBOOT]) reboot();
  if (wsTask[SCAN]) scanWiFi();
}