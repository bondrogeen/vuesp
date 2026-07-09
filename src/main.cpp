#include "./device/device.h"
#include "./discovery/discovery.h"
#include "./files/files.h"
#include "./gpio/gpio.h"
#include "./init/init.h"
#include "./runner/runner.h"
#include "./tasks/tasks.h"
#include "./webserver/webserver.h"

#if defined(ESP8266)
uint32_t id = ESP.getChipId();
#elif defined(ESP32)
uint32_t id = (uint32_t)(ESP.getEfuseMac() >> 24);
#endif

uint32_t now;
uint32_t lastTimeMain;

uint8_t isSetup = false;
uint8_t isInit = false;
uint8_t isFirst = false;

void setup() {
  Serial.begin(115200);
  initFS();
  initEEprom();
}

void logProvider(const char* message) {
  Serial.println(message);
}

bool loadScriptFromFS(uint8_t id, char* buffer, uint16_t& len) {
  File file = LittleFS.open("/www/scripts.txt", "r");
  if (!file) {
    Serial.print("[Load] id: ");
    Serial.println(id);
    return false;
  }
  while (file.available()) {
    String line = file.readStringUntil('\n');
    line.trim();
    if (line.length() == 0) continue;
    int colon = line.indexOf(':');
    if (colon == -1) continue;

    int fileId = line.substring(0, colon).toInt();
    String script = line.substring(colon + 1);

    if (fileId == id) {
      strcpy(buffer, script.c_str());
      len = script.length();
      file.close();
      Serial.printf("[Load] Script %d loaded (%d bytes)\n", id, len);
      return true;
    }
  }

  file.close();
  Serial.printf("[Load] Script %d not found\n", id);
  return false;
}

void setupFirst() {
  setupFirstDevice();
  setupFirstGPIO();
}

void setupDelay() {
  infoFS.id = id;
  infoFS.board = DEF_BOARD;
  char nameDevice[20];
  sprintf(nameDevice, "%s%02X", DEF_DEVICE_NAME, id);
  strcpy(infoFS.name, nameDevice);

  getInfo(&infoFS);
  loadConfig(settings, id);
  initWiFi();
  setupDevice();
  setupServer();
  setupDiscovery();

  scriptRunner.setLoadProvider(loadScriptFromFS);
  scriptRunner.setLogProvider(logProvider);
  scriptRunner.runScript(1);

  // String script = String("local x = 10 local y = 25 local z = x + y print('Sum of x+y =',z)");
  // Serial.println(lua.Lua_dostring(&script));
}

void loop() {
  now = millis();
  if (!isFirst) {
    setupFirst();
    isFirst = true;
  }

  if (!isSetup && now > 100) {
    setupDelay();
    isSetup = true;
  }

  if (isSetup) {
    scriptRunner.update();
    loopDiscovery(now);
    loopServer(now);
    loopTask(now);
    loopGPIO(now);
    loopDevice(now);
    loopWiFi(now);
  }

  if (now - lastTimeMain > 1000) {
    lastTimeMain = now;
    infoFS.uptime++;
  }
}