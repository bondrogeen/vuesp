#include <ArduinoOTA.h>
#ifdef ESP32
  #include <FS.h>
  #include <WiFi.h>
  #include <AsyncTCP.h>
#elif defined(ESP8266)
  #include <ESP8266WiFi.h>
  #include <ESPAsyncTCP.h>
  // #include <ESP8266mDNS.h>
#endif
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include "LittleFS.h"


#define RES_TYPE_JSON "application/json"

#include "./const/service_page.h"
#include "./const/struct.h"

WiFiClient WiFIclient;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *client;

#if defined(ESP8266)
uint32_t id = ESP.getChipId();
#elif defined(ESP32)
uint32_t id =  = ESP.getEfuseMac();
#endif

uint8_t wsClient = 0;
uint8_t wsTask[16];
uint8_t wsConnected = false;

uint32_t now;
uint32_t lastTime = 0;
FSInfo fs_info;


String status (uint8_t state) {
  return (state) ? "{\"state\":true}" : "{\"state\":false}";
}

void getFile(char* name) {
  Dir dir = LittleFS.openDir(name);
  while(dir.next()){
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
  Serial.println("Rebooting...");
  delay(100);
  ESP.restart();
}

void getInfoFS() {
  // info.totalBytes = SPIFFS.totalBytes();
  // info.usedBytes = SPIFFS.usedBytes();
  LittleFS.info(fs_info);
  info_fs.totalBytes = fs_info.totalBytes;
  info_fs.usedBytes = fs_info.usedBytes;
  ws.binary(wsClient, (uint8_t *)&info_fs, sizeof(info_fs));
  wsTask[INFO] = OFF;
  return;
}

void scanWiFi() {
  int n = WiFi.scanNetworks();
  for (int i = 0; i < n; i++) {
    scan.id = i;
    memset(scan.name, 0, sizeof(scan.name));
    WiFi.SSID(i).toCharArray(scan.name, 32);
    ws.binary(wsClient, (uint8_t *)&scan, sizeof(scan));
  };
  wsTask[SCAN] = OFF;
}

void saveConfig() {
  for (unsigned int t = 0; t < sizeof(storage); t++) {
    EEPROM.write(CONFIG_START + t, *((char *)&storage + t));
  }
  Serial.print("Save config");
  EEPROM.commit();
}

void loadConfig() {
  if (EEPROM.read(CONFIG_START + 1) == storage.version[0] &&
      EEPROM.read(CONFIG_START + 2) == storage.version[1] &&
      EEPROM.read(CONFIG_START + 3) == storage.version[2])
  {
    for (unsigned int t = 0; t < sizeof(storage); t++)
      *((char *)&storage + t) = EEPROM.read(CONFIG_START + t);
    Serial.print("Load config");
  } else {
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
          if (info->message_opcode == WS_TEXT) client->text("I got your text message");
          else client->binary("I got your binary message");
        }
      }
    }
  }
}

void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) request->_tempFile = LittleFS.open("/" + filename, "w");
  if (len) request->_tempFile.write(data, len);
  if (final) {
    request->_tempFile.close();
    request->send(200, RES_TYPE_JSON, status(1));
  }
}

void onUpdate(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if(!index){
    progress.size = index;
    ws.binaryAll((uint8_t *)&progress, sizeof(progress));
    Update.runAsync(true);

    int cmd = (filename == "littlefs.bin") ? U_FS : U_FLASH;
    size_t fsSize = ((size_t) &_FS_end - (size_t) &_FS_start);
    uint32_t maxSketchSpace = (ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000;
    if (!Update.begin((cmd == U_FS) ? fsSize : maxSketchSpace, cmd)){
      Update.printError(Serial);
      return request->send(400, RES_TYPE_JSON, status(0));
    }
  }
  if(!Update.hasError()){
    if(Update.write(data, len) != len){
      Update.printError(Serial);
    }
    progress.size = index;
    ws.binaryAll((uint8_t *)&progress, sizeof(progress));
  }
  if(final){
    if(Update.end(true)){
      progress.size = index+len;
      ws.binaryAll((uint8_t *)&progress, sizeof(progress));
    } else {
      Update.printError(Serial);
    }
  }
}

void initLittleFS() {
  if (!LittleFS.begin()) {
    if (LittleFS.format()) {
      Serial.println(F("Filesystem formatted!"));
    }
  }
}

void setup () {
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  EEPROM.begin(256);
  loadConfig();
  initLittleFS();

  if (storage.wifiMode) {
    if (!storage.wifiDhcp) {
      WiFi.config(storage.wifiIp, storage.wifiGeteway, storage.wifiSubnet, storage.wifiDns);
    }
    if (storage.wifiMode == WIFI_STA) {
      Serial.println("WIFI_STA");
      WiFi.mode(WIFI_STA);
      WiFi.begin(storage.wifiSsid, storage.wifiPass);
    } else if (storage.wifiMode == WIFI_AP) {
      Serial.println("WIFI_AP");
      WiFi.mode(WIFI_AP);
      WiFi.softAP(storage.wifiSsid, storage.wifiPass);
    }  
    Serial.println("END");
  }

  ws.onEvent(onWsEvent);

  server.addHandler(&ws);

  // #ifdef ESP32
  //   server.addHandler(new SPIFFSEditor(SPIFFS, storage.httpLogin, storage.httpPass));
  // #elif defined(ESP8266)
  //   server.addHandler(new SPIFFSEditor(storage.httpLogin, storage.httpPass));
  // #endif

  if (storage.httpMode) {
    server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html").setCacheControl("max-age=600").setAuthentication(storage.httpLogin, storage.httpPass);
  } else {
    server.serveStatic("/", LittleFS, "/").setCacheControl("max-age=600").setDefaultFile("index.html");
  }
  
  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) { 
    request->send(200); 
  }, onUpload);

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    uint8_t isReboot = !Update.hasError();
    if (isReboot) wsTask[REBOOT] = ON;
    AsyncWebServerResponse *response = request->beginResponse(200, RES_TYPE_JSON, status(isReboot));
    response->addHeader("Connection", "close");
    request->send(response);
  },onUpdate);

  server.on("/service", HTTP_GET, [](AsyncWebServerRequest *request) {
    if(!request->authenticate(storage.httpLogin, storage.httpPass)) return request->requestAuthentication();
    AsyncWebServerResponse *response = request->beginResponse_P(200, "text/html", service_gz, sizeof(service_gz));
    response->addHeader("Content-Encoding", "gzip");
    request->send(response);
  });

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->redirect("/service");
  });

  server.on("*", HTTP_ANY, [](AsyncWebServerRequest *request){
      request->redirect("/");
  });

  server.begin();
}

void loop() {
  now = millis();
  if(wsConnected) {
    if (now - lastTime > 1000) {
      lastTime = now;
      ws.binaryAll((uint8_t *)&ping, sizeof(ping));
      ws.cleanupClients();
    }
  }
  if (wsTask[SETTINGS]) {
    ws.binary(wsClient, (uint8_t *)&storage, sizeof(storage));
    wsTask[SETTINGS] = OFF;
    return;
  }
  if (wsTask[INFO]) {
    getInfoFS();
  }
  if (wsTask[FILES]) {
    getFile(files.name);
  }
  if (wsTask[REBOOT]) {
    reboot();
  }
  if (wsTask[SCAN]) {
    scanWiFi();
  }
}