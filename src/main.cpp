#include <ArduinoOTA.h>
#ifdef ESP32
#include <FS.h>
#include <SPIFFS.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESP8266mDNS.h>
#endif
#include <EEPROM.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFSEditor.h>

#define DEF_EEPROM_WRITE_TIME 30
#define DEF_EEPROM_SIZE 4096

#define CONFIG_START 32
#define INDEX_START_ADRESS 0

// Default struct settings
#define CONFIG_VERSION { 15,50,21 }

#define DEF_SERVER_URL "192.168.0.1"
#define DEF_SERVER_PORT 3001

#define DEF_ETH_DHCP 1
#define DEF_ETH_IP { 192,168,1,11 }
#define DEF_ETH_SUBNET { 255,255,255,0 }
#define DEF_ETH_GETEWAY { 192,168,1,1 }
#define DEF_ETH_DNS { 8,8,8,8 }

#define DEF_WIFI_DHCP 1
#define DEF_WIFI_IP { 192,168,1,12 }
#define DEF_WIFI_SUBNET { 255,255,255,0 }
#define DEF_WIFI_GETEWAY { 192,168,1,1 }
#define DEF_WIFI_DNS { 8,8,8,8 }

// #define DEF_WIFI_MODE WIFI_STA
#define DEF_WIFI_MODE WIFI_AP
#define DEF_WIFI_SSID ""
#define DEF_WIFI_PASS ""

#define DEF_HTTP_MODE 1
#define DEF_HTTP_LOGIN "admin"
#define DEF_HTTP_PASS "admin"

#define DEF_DEVICE_NAME "esp-1"

#define DEF_DEVICE_FIRMWARE {0,0,6}


WiFiClient WiFIclient;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");
AsyncWebSocketClient *client;

enum ws_comm {
  OFF,
  ON,
  ERASE,
  SETTINGS,
  SAVE,
  REBOOT,
  INFO,
  PING,
  TEST
};

uint8_t wsClient = 0;
uint8_t wsTask[16];
uint8_t wsConnected = false;
uint32_t wsTime = 0;

uint32_t now;
uint32_t lastTime = 0;

// struct Info {
//   uint8_t init;
//   uint8_t firmware[3];
//   uint32_t totalBytes;
//   uint32_t usedBytes;
// } info = {
//   INFO,
//   DEF_DEVICE_FIRMWARE,
//   0,
//   0
// };

struct Ping {
  uint8_t init;
} ping = {
  PING
};

struct StoreStruct {
  uint8_t init;
  uint8_t version[3];
  char serverUrl[32];
  uint16_t serverPort;

  uint8_t ethDhcp;
  uint8_t ethIp[4];
  uint8_t ethSubnet[4];
  uint8_t ethGeteway[4];
  uint8_t ethDns[4];

  uint8_t wifiDhcp;
  uint8_t wifiIp[4];
  uint8_t wifiSubnet[4];
  uint8_t wifiGeteway[4];
  uint8_t wifiDns[4];

  uint8_t wifiMode;
  char wifiSsid[32];
  char wifiPass[32];

  uint8_t httpMode;
  char httpLogin[12];
  char httpPass[12];

  char deviceName[12];
} storage = {
    SETTINGS,
    CONFIG_VERSION,
    DEF_SERVER_URL,
    DEF_SERVER_PORT,
    DEF_ETH_DHCP,
    DEF_ETH_IP,
    DEF_ETH_SUBNET,
    DEF_ETH_GETEWAY,
    DEF_ETH_DNS,
    DEF_WIFI_DHCP,
    DEF_WIFI_IP,
    DEF_WIFI_SUBNET,
    DEF_WIFI_GETEWAY,
    DEF_WIFI_DNS,
    DEF_WIFI_MODE,
    DEF_WIFI_SSID,
    DEF_WIFI_PASS,
    DEF_HTTP_MODE,
    DEF_HTTP_LOGIN,
    DEF_HTTP_PASS,
    DEF_DEVICE_NAME
    };


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



// uint8_t send(uint8_t data) {
//   if (WiFIclient.connect(storage.serverUrl, storage.serverPort)) {
//     Serial.println("connection");
//     // while (client.available()) {
//     // char line = client.read();
//     // }

//     Serial.print("Requesting POST: ");
//     // Send request to the server:
//     WiFIclient.println("POST /api/v1/card HTTP/1.1");
//     WiFIclient.print("Host: ");
//     WiFIclient.println(storage.serverUrl);
//     WiFIclient.println("Accept: */*");
//     WiFIclient.println("Content-Type: application/json");
//     WiFIclient.print("Content-Length: ");
//     WiFIclient.println(data.length());
//     WiFIclient.println();
//     WiFIclient.print(data);
//     delay(100); // Can be changed
//     while (WiFIclient.available()) {
//       char line = WiFIclient.read();
//       Serial.print(line);
//     }
//     Serial.println(data);
//     if (WiFIclient.connected()) {
//       WiFIclient.stop(); // DISCONNECT FROM THE SERVER
//     }
//     return 1;
//   }
//   else {
//     Serial.println("not connection");
//     return 0;
//   }
// }


void WiFiEvent(WiFiEvent_t event) {
  Serial.printf("[WiFi-event] event: %d\n", event);
  // switch (event) {
  // case SYSTEM_EVENT_STA_GOT_IP:
  //   Serial.print("Obtained IP address: ");
  //   Serial.println(WiFi.localIP());
  //   break;
  // default:
  //   break;
  // }
}

void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  if (!index) {
    request->_tempFile = SPIFFS.open("/" + filename, "w");
  }
  if (len) request->_tempFile.write(data, len);
  if (final) {
    request->_tempFile.close();
  }
}

void setup () {
  Serial.begin(115200);
  WiFi.onEvent(WiFiEvent);
  EEPROM.begin(256);
  loadConfig();

  if (!SPIFFS.begin()) {
    Serial.print(F("[ WARN ] Formatting filesystem..."));
    if (SPIFFS.format()) {
      Serial.println(F("Filesystem formatted!"));
    } else {
      Serial.println(F(" failed!"));
      Serial.println(F("[ WARN ] Could not format filesystem!"));
    }
  }

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

  Serial.println("Start...");

  // pinMode(READ_LED_PIN, OUTPUT);
  // digitalWrite(READ_LED_PIN, LOW);

  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  #ifdef ESP32
    server.addHandler(new SPIFFSEditor(SPIFFS, storage.httpLogin, storage.httpPass));
  #elif defined(ESP8266)
    server.addHandler(new SPIFFSEditor(storage.httpLogin, storage.httpPass));
  #endif

  if (storage.httpMode) {
    server.serveStatic("/", SPIFFS, "/")
      .setDefaultFile("index.html")
      .setCacheControl("max-age=600")
      .setAuthentication(storage.httpLogin, storage.httpPass);
  } else {
    server.serveStatic("/", SPIFFS, "/")
      // .setCacheControl("max-age=600")
      .setDefaultFile("index.html");
  }
  
  server.onNotFound([](AsyncWebServerRequest *request) {
    request->send(404);
  });

  server.on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) { request->send(200); },onUpload);

  server.on("/upload", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>");
  });

  server.on("/reboot", HTTP_ANY, [](AsyncWebServerRequest *request) {
    request->send(200, "application/json", "{\"state\":true}");
    wsTask[REBOOT] = ON;
  });

  server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(200, "text/html", "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>");
  });

  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request) {
    uint8_t shouldReboot = !Update.hasError();
    if (shouldReboot) {
      wsTask[REBOOT] = ON;
    }
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", shouldReboot?"OK":"FAIL");
    response->addHeader("Connection", "close");
    request->send(response); }, [](AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
      if(!index){
        Serial.printf("Update Start: %s\n", filename.c_str());
        ws.text(wsClient, "Update Start");
        // Update.runAsync(true);
        if(!Update.begin((ESP.getFreeSketchSpace() - 0x1000) & 0xFFFFF000)){
          Update.printError(Serial);
        }
      }
      if(!Update.hasError()){
        if(Update.write(data, len) != len){
          Update.printError(Serial);
        }
        Serial.printf("Update Success: %uB\n", index+len);
      }
      if(final){
        if(Update.end(true)){
          ws.text(wsClient, "Update Success");
          Serial.printf("Update Success: %uB\n", index+len);
        } else {
          Update.printError(Serial);
        }
      } 
    });

  server.on("*", HTTP_ANY, [](AsyncWebServerRequest *request){
    request->redirect("/");
  });

  server.begin();
}

void loop() {
  now = millis();
  
  if(wsConnected) {
    wsTime = now;
    if (wsTask[SETTINGS]) {
      ws.binary(wsClient, (uint8_t *)&storage, sizeof(storage));
      wsTask[SETTINGS] = OFF;
      return;
    }
    // if (wsTask[INFO]) {
    //   info.totalBytes = SPIFFS.totalBytes();
    //   info.usedBytes = SPIFFS.usedBytes();
    //   ws.binary(wsClient, (uint8_t *)&info, sizeof(info));
    //   wsTask[INFO] = OFF;
    //   return;
    // }
    if (now - lastTime > 1000) {
      lastTime = now;
      ws.binaryAll((uint8_t *)&ping, sizeof(ping));
      ws.cleanupClients();
    }
  }
  if (wsTask[REBOOT]) {
    Serial.println("Rebooting...");
    delay(100);
    ESP.restart();
  }

}