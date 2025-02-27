#include "./include/device.h"
#include "./include/eeprom.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"
#include "./include/webserver.h"
// #include "./luas/LuaWrapper.h"

// LuaWrapper lua;

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

void setupFirst() {
  setupFirstDevice();
  setupFirstGPIO();
}

void setupDelay() {
  infoFS.id = id;
  getInfo(&infoFS);
  loadConfig(settings, id);
  initWiFi();
  setupDevice();
  setupServer();

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