#include "./device/device.h"
#include "./discovery/discovery.h"
#include "./files/files.h"
#include "./gpio/gpio.h"
#include "./init/init.h"
#include "./runner/runner.h"
#include "./setup/serialSetup.h"
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

void setupFirst() {
  setupFirstDevice();
  setupFirstGPIO();
}

void setupDelay() {
  infoFS.id = id;
  infoFS.board = DEF_BOARD;
  char nameDevice[40];
  snprintf(nameDevice, sizeof(nameDevice), "%s%02X", DEF_DEVICE_NAME, id);
  strncpy(infoFS.name, nameDevice, sizeof(infoFS.name) - 1);
  infoFS.name[sizeof(infoFS.name) - 1] = 0;

  getInfo(&infoFS);
  loadConfig(settings, id);
  initWiFi();
  setupGPIO();
  setupDevice();
  setupServer();
  setupDiscovery();
  setupRunner();

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
    loopDiscovery(now);
    loopServer(now);
    loopTask(now);
    loopGPIO(now);
    loopDevice(now);
    loopWiFi(now);
    loopRunner(now);
    loopSerialSetup();
  }

  if (now - lastTimeMain > 1000) {
    lastTimeMain = now;
    infoFS.uptime++;
  }
}