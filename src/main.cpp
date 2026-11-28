#include "./config/ConfigStore.h"
#include "./const/features.h"
#include "./diag/bootlog.h"
#include "./diag/diag.h"
#include "./device/device.h"
#if VUESP_DISCOVERY
#include "./discovery/discovery.h"
#endif
#include "./files/files.h"
#include "./gpio/gpio.h"
#include "./init/init.h"
#if VUESP_SCRIPT
#include "./runner/bytecode/BytecodeService.h"
#include "./runner/bytecode/ScriptProtocol.h"
#include "./runner/runner.h"
#endif
#if VUESP_MODBUS
#include "./modbus/modbus.h"
#endif
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
  bootLog();
  diagInit();
  initFS();
  initEEprom();
}

void setupFirst() {
  configStore.begin();
  setupFirstDevice();
  setupFirstGPIO();
#if VUESP_MODBUS
  setupFirstModbus();
#endif
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
#if VUESP_MODBUS
  setupModbus();
#endif
  setupServer();
#if VUESP_DISCOVERY
  setupDiscovery();
#endif
#if VUESP_SCRIPT
  setupRunner();
  bytecodeService.begin();
  scriptProtocol.begin();
#endif
}

void loop() {
  diagLoopBegin();
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
#if VUESP_DISCOVERY
    loopDiscovery(now);
#endif
    loopServer(now);
    loopTask(now);
    loopGPIO(now);
    loopDevice(now);
#if VUESP_MODBUS
    loopModbus(now);
#endif
    loopWiFi(now);
#if VUESP_SCRIPT
    bytecodeService.loop(now);
    scriptProtocol.loop();
#endif
    loopSerialSetup();
  }

  if (configStore.dirty()) {
    const TConfigStatus status = configStore.flush();
    if (status != TConfigStatus::Ok && status != TConfigStatus::Deferred) {
#if VUESP_LOG_LEVEL >= 1
      Serial.printf("[config] flush: %d\n", (int)status);
#endif
    }
  }

  if (now - lastTimeMain > 1000) {
    lastTimeMain = now;
    infoFS.uptime++;
#if VUESP_SCRIPT
    if (isSetup) {
      const uint32_t seconds = (uint32_t)infoFS.uptime;
      const int32_t hours = (int32_t)((seconds / 3600) % 24);
      const int32_t minutes = (int32_t)((seconds / 60) % 60);
      bytecodeService.vm().emitEvent("time", 2, hours, minutes);
    }
#endif
  }
  diagLoopEnd();

  // Yield the rest of the slot to Wi-Fi and system tasks. All timers here are
  // 10 ms or slower, so 1 ms does not change behaviour but stops the busy loop.
  delay(1);
}