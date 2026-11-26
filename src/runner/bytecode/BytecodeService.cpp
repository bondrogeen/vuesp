#include "./BytecodeService.h"

#if ENABLE_BYTECODE_VM

#include <Arduino.h>
#include <LittleFS.h>

#include <stdio.h>
#include <string.h>

#include "./BytecodeVerifier.h"

using namespace vuesp::bytecode;

namespace {

const FunctionSpec kFunctions[] = {
    {1, "fade", 3, 3, VALUE_INT},
    {2, "udp", 1, 2, VALUE_INT},
    {3, "http", 2, 5, VALUE_INT},
};

const EventSpec kEvents[] = {
    {1, "btn", 2},
    {2, "btn_c", 3},
    {3, "btn_l", 2},
    {4, "btn_r", 2},
};

const Capabilities kCaps = {
    1,
    VBC_MAX_CODE_BYTES,
    VBC_STACK_SIZE,
    {10, 10, 5, 2, 5, 40, 4, 4},
    VBC_MAX_STRING_LEN,
    VBC_MAX_ARRAY_LEN,
    32,
    300,
    3000,
    kFunctions,
    3,
    kEvents,
    4,
};

}  // namespace

BytecodeService bytecodeService;

String BytecodeService::pathFor(uint8_t id) const {
  return String("/www/scripts/") + String((unsigned)id) + ".vbc";
}

void BytecodeService::begin() {
  memset(_slots, 0, sizeof(_slots));
  _vm.setCapabilities(kCaps);

  if (!LittleFS.exists("/www/scripts")) {
    LittleFS.mkdir("/www/scripts");
  }

  Dir dir = LittleFS.openDir("/www/scripts");
  while (dir.next()) {
    const String name = dir.fileName();
    const int id = name.toInt();
    if (id <= 0 || id > 254) continue;
    if (findFreeSlot() < 0) break;
    loadFileToSlot((uint8_t)id);
  }
}

void BytecodeService::loop(uint32_t now) { _vm.update(now); }

int8_t BytecodeService::findSlot(uint8_t id) const {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (_slots[i].used && _slots[i].id == id) return (int8_t)i;
  }
  return -1;
}

int8_t BytecodeService::findFreeSlot() const {
  for (uint8_t i = 0; i < kCacheSlots; i++) {
    if (!_slots[i].used) return (int8_t)i;
  }
  return -1;
}

bool BytecodeService::loadFile(uint8_t id, uint8_t* buffer, uint16_t maxLen, uint16_t& len) {
  File file = LittleFS.open(pathFor(id), "r");
  if (!file) return false;
  const size_t size = file.size();
  if (size == 0 || size > maxLen) {
    file.close();
    return false;
  }
  const size_t read = file.read(buffer, size);
  file.close();
  if (read != size) return false;
  len = (uint16_t)size;
  return true;
}

int8_t BytecodeService::loadFileToSlot(uint8_t id) {
  int8_t slot = findFreeSlot();
  if (slot < 0) {
    for (uint8_t i = 0; i < kCacheSlots; i++) {
      if (_slots[i].used && !_vm.isScriptActive(_slots[i].id)) {
        _vm.removeScript(_slots[i].id);
        _slots[i].used = false;
        slot = (int8_t)i;
        break;
      }
    }
  }
  if (slot < 0) return -1;

  uint16_t len = 0;
  if (!loadFile(id, _slots[slot].data, sizeof(_slots[slot].data), len)) return -1;
  if (!_vm.loadScript(id, _slots[slot].data, len)) return -1;
  _slots[slot].used = true;
  _slots[slot].id = id;
  _slots[slot].len = len;
  return slot;
}

bool BytecodeService::saveScript(uint8_t id, const uint8_t* data, uint16_t len) {
  if (id > 254 || !data || len == 0 || len > sizeof(_slots[0].data)) return false;

  ContainerView view;
  ParseError parseError;
  if (!parseContainer(data, len, view, parseError)) return false;
  VerifyResult verify;
  if (!verifyContainer(view, kCaps, verify)) return false;

  const String path = pathFor(id);
  const String tmp = path + ".tmp";
  File file = LittleFS.open(tmp, "w");
  if (!file) return false;
  const size_t written = file.write(data, len);
  file.close();
  if (written != len) {
    LittleFS.remove(tmp);
    return false;
  }
  if (LittleFS.exists(path)) LittleFS.remove(path);
  if (!LittleFS.rename(tmp, path)) {
    LittleFS.remove(tmp);
    return false;
  }

  int8_t slot = findSlot(id);
  if (slot < 0) slot = findFreeSlot();
  if (slot >= 0) {
    memcpy(_slots[slot].data, data, len);
    _slots[slot].used = true;
    _slots[slot].id = id;
    _slots[slot].len = len;
    if (!_vm.loadScript(id, _slots[slot].data, len)) {
      _slots[slot].used = false;
    }
  }
  return true;
}

bool BytecodeService::removeScript(uint8_t id) {
  _vm.removeScript(id);
  const int8_t slot = findSlot(id);
  if (slot >= 0) _slots[slot].used = false;
  const String path = pathFor(id);
  return LittleFS.exists(path) ? LittleFS.remove(path) : false;
}

bool BytecodeService::runScript(uint8_t id) {
  int8_t slot = findSlot(id);
  if (slot < 0) slot = loadFileToSlot(id);
  if (slot < 0) return false;
  if (!_vm.loadScript(id, _slots[slot].data, _slots[slot].len)) return false;
  return _vm.runScript(id) >= 0;
}

bool BytecodeService::stopScript(uint8_t id) { return _vm.stopScript(id); }

void BytecodeService::stopAll() { _vm.stopAll(); }

bool BytecodeService::emitEvent(const char* name, uint8_t count, const int32_t* params) {
  if (!name) return false;
  for (uint8_t i = 0; i < sizeof(kEvents) / sizeof(kEvents[0]); i++) {
    if (strcmp(kEvents[i].name, name) == 0) {
      return _vm.emitEvent(kEvents[i].id, count, params);
    }
  }
  return false;
}

bool BytecodeService::readScript(uint8_t id, uint8_t* buffer, uint16_t maxLen, uint16_t& len) {
  const int8_t slot = findSlot(id);
  if (slot >= 0) {
    if (_slots[slot].len > maxLen) return false;
    memcpy(buffer, _slots[slot].data, _slots[slot].len);
    len = _slots[slot].len;
    return true;
  }
  return loadFile(id, buffer, maxLen, len);
}

uint8_t BytecodeService::scriptCount() const { return _vm.scriptCount(); }

String BytecodeService::capabilitiesJson() const {
  return String(
      R"json({"vm":1,"maxScripts":15,"maxCodeBytes":1024,"maxStack":64,"vars":{"u":10,"i":10,"f":5,"s":2,"a":5,"p":40,"e":4,"c":4},"maxStringLen":32,"maxArrayLen":64,"functions":[{"id":1,"name":"fade","min":3,"max":3,"ret":"int"},{"id":2,"name":"udp","min":1,"max":2,"ret":"int"},{"id":3,"name":"http","min":2,"max":5,"ret":"int"}],"events":[{"id":1,"name":"btn","params":2},{"id":2,"name":"btn_c","params":3},{"id":3,"name":"btn_l","params":2},{"id":4,"name":"btn_r","params":2}],"dataVars":[{"name":"$display","kind":"string"}],"limits":{"instrPerTick":32,"usPerTick":300,"updateBudgetUs":3000}})json");
}

String BytecodeService::listJson() const {
  String json = "[";
  bool first = true;
  Dir dir = LittleFS.openDir("/www/scripts");
  while (dir.next()) {
    const String name = dir.fileName();
    const int id = name.toInt();
    if (id <= 0 || id > 254) continue;
    File file = dir.openFile("r");
    const size_t size = file ? file.size() : 0;
    if (file) file.close();
    if (!first) json += ',';
    first = false;
    json += "{\"id\":";
    json += id;
    json += ",\"size\":";
    json += (unsigned)size;
    json += '}';
  }
  json += ']';
  return json;
}

String BytecodeService::statusJson() const {
  char buffer[160];
  snprintf(buffer, sizeof(buffer),
           "{\"active\":%u,\"scripts\":%u,\"error\":%u,\"errorScript\":%u,\"errorPc\":%u}",
           (unsigned)_vm.activeContexts(), (unsigned)_vm.scriptCount(),
           (unsigned)_vm.lastError(), (unsigned)_vm.lastErrorScript(),
           (unsigned)_vm.lastErrorPc());
  return String(buffer);
}

#endif  // ENABLE_BYTECODE_VM
