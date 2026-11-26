#include "./BytecodeService.h"

#if ENABLE_BYTECODE_VM

#include <Arduino.h>
#include <LittleFS.h>

#include <stdio.h>
#include <string.h>

#include "./BytecodeVerifier.h"

using namespace vuesp::bytecode;

namespace {

const char kPackPath[] = "/www/scripts.bc";
const uint16_t kIoChunk = 64;

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

bool readFull(File& file, uint8_t* buffer, uint16_t len) {
  uint16_t offset = 0;
  while (offset < len) {
    const size_t read = file.read(buffer + offset, len - offset);
    if (read == 0) return false;
    offset = (uint16_t)(offset + read);
  }
  return true;
}

bool writeFull(File& file, const uint8_t* data, uint16_t len) {
  uint16_t offset = 0;
  while (offset < len) {
    const size_t written = file.write(data + offset, len - offset);
    if (written == 0) return false;
    offset = (uint16_t)(offset + written);
  }
  return true;
}

void buildHeader(uint8_t* header, uint8_t count, uint16_t tocSize,
                 uint32_t tocCrc) {
  memcpy(header, "VSP1", 4);
  header[4] = kPackVersion;
  header[5] = count;
  header[6] = 0;
  header[7] = 0;
  writeU16(header + 8, tocSize);
  writeU32(header + 10, tocCrc);
}

bool copyCode(File& in, File& out, uint16_t offset, uint16_t len,
              uint32_t expectedCrc) {
  if (!in.seek(offset)) return false;

  uint8_t buffer[kIoChunk];
  uint32_t crc = 0xffffffffu;
  uint16_t remaining = len;
  while (remaining > 0) {
    const uint16_t chunk = remaining > sizeof(buffer) ? sizeof(buffer) : remaining;
    if (!readFull(in, buffer, chunk)) return false;
    crc = crc32Update(crc, buffer, chunk);
    if (!writeFull(out, buffer, chunk)) return false;
    remaining = (uint16_t)(remaining - chunk);
  }
  return (crc ^ 0xffffffffu) == expectedCrc;
}

}  // namespace

BytecodeService bytecodeService;

void BytecodeService::begin() {
  memset(_slots, 0, sizeof(_slots));
  memset(_refs, 0, sizeof(_refs));
  memset(&_packView, 0, sizeof(_packView));
  _tocLen = 0;
  _packCount = 0;
  _packValid = false;
  _packError = PackError::NONE;

  _vm.setCapabilities(kCaps);
  _vm.setScriptResolver(&BytecodeService::resolveThunk, this);
  loadToc();
  if (_packValid) runScript("boot");
}

void BytecodeService::loop(uint32_t now) { _vm.update(now); }

bool BytecodeService::loadToc() {
  _packValid = false;
  _packCount = 0;
  _packError = PackError::NONE;
  _tocLen = 0;
  memset(&_packView, 0, sizeof(_packView));

  const String path = kPackPath;
  const String tmp = path + ".tmp";
  const String bak = path + ".bak";
  if (LittleFS.exists(tmp)) LittleFS.remove(tmp);
  if (!LittleFS.exists(path) && LittleFS.exists(bak)) {
    LittleFS.rename(bak, path);
  } else if (LittleFS.exists(bak)) {
    LittleFS.remove(bak);
  }
  if (!LittleFS.exists(path)) {
    memset(_refs, 0, sizeof(_refs));
    _packValid = true;
    return true;
  }

  File file = LittleFS.open(path, "r");
  if (!file) {
    _packError = PackError::SHORT;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }

  const size_t fileSize = file.size();
  if (fileSize < kPackHeaderSize || fileSize > 0xffffu) {
    file.close();
    _packError = fileSize > 0xffffu ? PackError::BOUNDS : PackError::SHORT;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }
  if (!readFull(file, _toc, kPackHeaderSize)) {
    file.close();
    _packError = PackError::SHORT;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }

  const uint16_t tocSize = readU16(_toc + 8);
  if (tocSize > (uint16_t)kPackMaxEntries * kPackEntrySize) {
    file.close();
    _packError = PackError::TOC_SIZE;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }
  const uint16_t need = (uint16_t)(kPackHeaderSize + tocSize);
  if ((uint32_t)need > fileSize) {
    file.close();
    _packError = PackError::BOUNDS;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }
  if (!file.seek(0) || !readFull(file, _toc, need)) {
    file.close();
    _packError = PackError::SHORT;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }
  file.close();

  _tocLen = need;
  if (!parsePackToc(_toc, _tocLen, (uint32_t)fileSize, _packView, _packError)) {
    _packView.toc = nullptr;
    memset(_refs, 0, sizeof(_refs));
    return false;
  }

  _packCount = _packView.header.count;
  _packValid = true;
  rebuildRefs();
  return true;
}

void BytecodeService::rebuildRefs() {
  ScriptRef next[kPackMaxEntries];
  memset(next, 0, sizeof(next));

  for (uint8_t i = 0; i < _packCount; i++) {
    PackEntry entry;
    if (!getPackEntry(_packView, i, entry)) continue;
    for (uint8_t j = 0; j < kPackMaxEntries; j++) {
      if (_refs[j].used &&
          memcmp(_refs[j].name, entry.name, kPackNameLen) == 0) {
        next[i].used = true;
        next[i].id = _refs[j].id;
        memcpy(next[i].name, entry.name, kPackNameLen);
        break;
      }
    }
  }

  for (uint8_t i = 0; i < _packCount; i++) {
    if (next[i].used) continue;
    uint8_t id = 0;
    for (uint8_t candidate = 1; candidate <= kPackMaxEntries && id == 0;
         candidate++) {
      bool used = false;
      for (uint8_t j = 0; j < kPackMaxEntries; j++) {
        if (next[j].used && next[j].id == candidate) {
          used = true;
          break;
        }
      }
      if (!used) id = candidate;
    }
    if (id == 0) break;

    PackEntry entry;
    if (!getPackEntry(_packView, i, entry)) continue;
    next[i].used = true;
    next[i].id = id;
    memcpy(next[i].name, entry.name, kPackNameLen);
  }

  memcpy(_refs, next, sizeof(next));
}

int8_t BytecodeService::findRefByKey(const uint8_t name[kPackNameLen]) const {
  for (uint8_t i = 0; i < kPackMaxEntries; i++) {
    if (_refs[i].used && memcmp(_refs[i].name, name, kPackNameLen) == 0) {
      return (int8_t)i;
    }
  }
  return -1;
}

int8_t BytecodeService::findRef(const char* name) const {
  uint8_t key[kPackNameLen];
  if (!packNormalizeName(name, key)) return -1;
  return findRefByKey(key);
}

int8_t BytecodeService::findRefById(uint8_t id) const {
  for (uint8_t i = 0; i < kPackMaxEntries; i++) {
    if (_refs[i].used && _refs[i].id == id) return (int8_t)i;
  }
  return -1;
}

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

bool BytecodeService::readBlob(uint16_t index, uint8_t* buffer, uint16_t maxLen,
                               uint16_t& len, VmError& error) {
  error = VmError::NONE;
  if (!_packValid || !_packView.toc || !buffer) {
    error = VmError::PARSE;
    return false;
  }

  PackEntry entry;
  if (!getPackEntry(_packView, index, entry)) {
    error = VmError::PARSE;
    return false;
  }
  if (entry.codeLen > maxLen) {
    error = VmError::BOUNDS;
    return false;
  }

  File file = LittleFS.open(kPackPath, "r");
  if (!file) {
    error = VmError::PARSE;
    return false;
  }
  const bool ok = file.seek(entry.codeOff) &&
                  readFull(file, buffer, entry.codeLen);
  file.close();
  if (!ok) {
    error = VmError::PARSE;
    return false;
  }

  len = entry.codeLen;
  if (!verifyPackCode(_packView, index, buffer, len)) {
    error = VmError::CODE_CRC;
    return false;
  }
  return true;
}

int8_t BytecodeService::loadEntryToSlot(uint16_t index, uint8_t id,
                                        VmError& error) {
  error = VmError::NONE;
  int8_t slot = findSlot(id);
  if (slot >= 0) return slot;

  slot = findFreeSlot();
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
  if (slot < 0) {
    error = VmError::NO_CACHE;
    return -1;
  }

  uint16_t len = 0;
  if (!readBlob(index, _slots[slot].data, sizeof(_slots[slot].data), len,
                error)) {
    return -1;
  }
  _slots[slot].used = true;
  _slots[slot].id = id;
  _slots[slot].len = len;
  return slot;
}

bool BytecodeService::saveScript(const char* name, const uint8_t* data,
                                 uint16_t len) {
  uint8_t key[kPackNameLen];
  if (!packNormalizeName(name, key) || !data || len == 0 ||
      len > kMaxCodeBytes) {
    return false;
  }

  ContainerView view;
  ParseError parseError;
  if (!parseContainer(data, len, view, parseError)) return false;
  VerifyResult verify;
  if (!verifyContainer(view, kCaps, verify)) return false;

  loadToc();
  return savePack(key, data, len);
}

bool BytecodeService::savePack(const uint8_t name[kPackNameLen],
                               const uint8_t* data, uint16_t len) {
  const uint8_t oldCount = _packValid ? _packCount : 0;
  uint16_t replaceIndex = 0;
  bool replacing = false;
  for (uint8_t i = 0; i < oldCount; i++) {
    PackEntry entry;
    if (!getPackEntry(_packView, i, entry)) return false;
    if (memcmp(entry.name, name, kPackNameLen) == 0) {
      replaceIndex = i;
      replacing = true;
      break;
    }
  }
  if (!replacing && oldCount >= kPackMaxEntries) return false;

  const uint8_t newCount = (uint8_t)(oldCount + (replacing ? 0 : 1));
  const uint16_t newTocSize = (uint16_t)newCount * kPackEntrySize;
  uint8_t toc[kPackMaxEntries * kPackEntrySize];
  memset(toc, 0, sizeof(toc));

  uint16_t codeOff = (uint16_t)(kPackHeaderSize + newTocSize);
  uint8_t outIndex = 0;
  for (uint8_t i = 0; i < oldCount; i++) {
    PackEntry entry;
    if (!getPackEntry(_packView, i, entry)) return false;

    const bool target = replacing && i == replaceIndex;
    uint8_t* slot = toc + (uint16_t)outIndex * kPackEntrySize;
    memcpy(slot, entry.name, kPackNameLen);
    slot[16] = entry.flags;
    const uint16_t codeLen = target ? len : entry.codeLen;
    writeU16(slot + 17, codeOff);
    writeU16(slot + 19, codeLen);
    writeU32(slot + 21, target ? crc32(data, len) : entry.codeCrc);
    codeOff = (uint16_t)(codeOff + codeLen);
    outIndex++;
  }

  if (!replacing) {
    uint8_t* slot = toc + (uint16_t)outIndex * kPackEntrySize;
    memcpy(slot, name, kPackNameLen);
    slot[16] = 0;
    writeU16(slot + 17, codeOff);
    writeU16(slot + 19, len);
    writeU32(slot + 21, crc32(data, len));
    codeOff = (uint16_t)(codeOff + len);
  }

  uint8_t header[kPackHeaderSize];
  buildHeader(header, newCount, newTocSize, crc32(toc, newTocSize));

  const String path = kPackPath;
  const String tmp = path + ".tmp";
  File out = LittleFS.open(tmp, "w");
  if (!out) return false;

  bool ok = writeFull(out, header, sizeof(header)) &&
            writeFull(out, toc, newTocSize);

  File in;
  if (ok && oldCount > 0) {
    in = LittleFS.open(path, "r");
    if (!in) ok = false;
  }
  if (ok && oldCount > 0) {
    for (uint8_t i = 0; i < oldCount; i++) {
      PackEntry entry;
      if (!getPackEntry(_packView, i, entry)) {
        ok = false;
        break;
      }
      if (replacing && i == replaceIndex) {
        if (!writeFull(out, data, len)) ok = false;
      } else if (!copyCode(in, out, entry.codeOff, entry.codeLen,
                           entry.codeCrc)) {
        ok = false;
      }
      if (!ok) break;
    }
  }
  if (ok && oldCount == 0) {
    if (!writeFull(out, data, len)) ok = false;
  } else if (ok && !replacing) {
    if (!writeFull(out, data, len)) ok = false;
  }

  if (ok && out.size() != codeOff) ok = false;
  if (in) in.close();
  out.close();

  if (!ok) {
    LittleFS.remove(tmp);
    return false;
  }
  if (!commitPack(tmp)) return false;

  if (!loadToc()) return false;

  const int8_t ref = findRefByKey(name);
  if (ref >= 0) {
    const int8_t slot = findSlot(_refs[ref].id);
    if (slot >= 0) {
      memcpy(_slots[slot].data, data, len);
      _slots[slot].len = len;
      if (!_vm.loadScript(_slots[slot].id, _slots[slot].data, len)) {
        _slots[slot].used = false;
      }
    }
  }
  return true;
}

bool BytecodeService::removeScript(const char* name) {
  if (!loadToc() || !_packValid) return false;

  uint16_t index = 0;
  if (!findPackEntry(_packView, name, index)) return false;

  uint8_t removedId = 0;
  const int8_t ref = findRef(name);
  if (ref >= 0) removedId = _refs[ref].id;

  return removePackEntry(index, removedId);
}

bool BytecodeService::removePackEntry(uint16_t index, uint8_t removedId) {
  if (!_packValid || index >= _packCount) return false;

  const uint8_t newCount = (uint8_t)(_packCount - 1);
  const uint16_t newTocSize = (uint16_t)newCount * kPackEntrySize;
  uint8_t toc[kPackMaxEntries * kPackEntrySize];
  memset(toc, 0, sizeof(toc));

  uint16_t codeOff = (uint16_t)(kPackHeaderSize + newTocSize);
  uint8_t outIndex = 0;
  for (uint8_t i = 0; i < _packCount; i++) {
    if (i == index) continue;
    PackEntry entry;
    if (!getPackEntry(_packView, i, entry)) return false;
    uint8_t* slot = toc + (uint16_t)outIndex * kPackEntrySize;
    memcpy(slot, entry.name, kPackNameLen);
    slot[16] = entry.flags;
    writeU16(slot + 17, codeOff);
    writeU16(slot + 19, entry.codeLen);
    writeU32(slot + 21, entry.codeCrc);
    codeOff = (uint16_t)(codeOff + entry.codeLen);
    outIndex++;
  }

  uint8_t header[kPackHeaderSize];
  buildHeader(header, newCount, newTocSize, crc32(toc, newTocSize));

  const String path = kPackPath;
  const String tmp = path + ".tmp";
  File out = LittleFS.open(tmp, "w");
  if (!out) return false;

  bool ok = writeFull(out, header, sizeof(header)) &&
            writeFull(out, toc, newTocSize);
  File in;
  if (ok && _packCount > 1) {
    in = LittleFS.open(path, "r");
    if (!in) ok = false;
  }
  if (ok && _packCount > 1) {
    for (uint8_t i = 0; i < _packCount; i++) {
      if (i == index) continue;
      PackEntry entry;
      if (!getPackEntry(_packView, i, entry)) {
        ok = false;
        break;
      }
      if (!copyCode(in, out, entry.codeOff, entry.codeLen, entry.codeCrc)) {
        ok = false;
        break;
      }
    }
  }

  if (ok && out.size() != codeOff) ok = false;
  if (in) in.close();
  out.close();

  if (!ok) {
    LittleFS.remove(tmp);
    return false;
  }
  if (!commitPack(tmp)) return false;

  if (removedId != 0) {
    _vm.removeScript(removedId);
    const int8_t slot = findSlot(removedId);
    if (slot >= 0) _slots[slot].used = false;
  }
  return loadToc();
}

bool BytecodeService::commitPack(const String& tmp) {
  const String path = kPackPath;
  const String bak = path + ".bak";
  if (!LittleFS.exists(tmp)) return false;
  LittleFS.remove(bak);

  if (LittleFS.rename(tmp, path)) {
    LittleFS.remove(bak);
    return true;
  }
  if (!LittleFS.exists(path)) return false;
  if (!LittleFS.rename(path, bak)) return false;
  if (!LittleFS.rename(tmp, path)) {
    LittleFS.rename(bak, path);
    return false;
  }
  LittleFS.remove(bak);
  return true;
}

bool BytecodeService::resolveThunk(void* userData, const char* name, uint8_t& id,
                                   const uint8_t*& data, uint16_t& size,
                                   VmError& error) {
  if (!userData) {
    error = VmError::NO_SCRIPT;
    return false;
  }
  return static_cast<BytecodeService*>(userData)
      ->resolveScript(name, id, data, size, error);
}

bool BytecodeService::resolveScript(const char* name, uint8_t& id,
                                    const uint8_t*& data, uint16_t& size,
                                    VmError& error) {
  error = VmError::NO_SCRIPT;
  if (!_packValid) return false;

  uint16_t index = 0;
  if (!findPackEntry(_packView, name, index)) return false;
  const int8_t ref = findRef(name);
  if (ref < 0) return false;

  id = _refs[ref].id;
  error = VmError::NONE;
  const int8_t slot = loadEntryToSlot(index, id, error);
  if (slot < 0) return false;

  data = _slots[slot].data;
  size = _slots[slot].len;
  return true;
}

bool BytecodeService::runScript(const char* name) {
  if (!_packValid) return false;

  uint16_t index = 0;
  if (!findPackEntry(_packView, name, index)) return false;
  const int8_t ref = findRef(name);
  if (ref < 0) return false;

  const uint8_t id = _refs[ref].id;
  VmError loadError = VmError::NONE;
  const int8_t slot = loadEntryToSlot(index, id, loadError);
  if (slot < 0) {
    _vm.reportError(loadError == VmError::NONE ? VmError::NO_SCRIPT : loadError, id, 0);
    return false;
  }
  if (!_vm.loadScript(id, _slots[slot].data, _slots[slot].len)) {
    _slots[slot].used = false;
    _vm.removeScript(id);
    return false;
  }
  return _vm.runScript(id) >= 0;
}

bool BytecodeService::stopScript(const char* name) {
  const int8_t ref = findRef(name);
  if (ref < 0) return false;
  return _vm.stopScript(_refs[ref].id);
}

void BytecodeService::stopAll() { _vm.stopAll(); }

bool BytecodeService::emitEvent(const char* name, uint8_t count,
                                const int32_t* params) {
  if (!name) return false;
  for (uint8_t i = 0; i < sizeof(kEvents) / sizeof(kEvents[0]); i++) {
    if (strcmp(kEvents[i].name, name) == 0) {
      return _vm.emitEvent(kEvents[i].id, count, params);
    }
  }
  return false;
}

bool BytecodeService::readScript(const char* name, uint8_t* buffer,
                                 uint16_t maxLen, uint16_t& len) {
  if (!_packValid || !buffer) return false;

  const int8_t ref = findRef(name);
  if (ref < 0) return false;
  const int8_t slot = findSlot(_refs[ref].id);
  if (slot >= 0) {
    if (_slots[slot].len > maxLen) return false;
    memcpy(buffer, _slots[slot].data, _slots[slot].len);
    len = _slots[slot].len;
    return true;
  }

  uint16_t index = 0;
  if (!findPackEntry(_packView, name, index)) return false;
  VmError error = VmError::NONE;
  return readBlob(index, buffer, maxLen, len, error);
}

uint8_t BytecodeService::scriptCount() const { return _vm.scriptCount(); }

uint8_t BytecodeService::savedCount() const { return _packCount; }

String BytecodeService::capabilitiesJson() const {
  return String(
      R"json({"vm":1,"maxScripts":15,"maxCodeBytes":1024,"maxStack":64,"vars":{"u":10,"i":10,"f":5,"s":2,"a":5,"p":40,"e":4,"c":4},"maxStringLen":32,"maxArrayLen":64,"functions":[{"id":1,"name":"fade","min":3,"max":3,"ret":"int"},{"id":2,"name":"udp","min":1,"max":2,"ret":"int"},{"id":3,"name":"http","min":2,"max":5,"ret":"int"}],"events":[{"id":1,"name":"btn","params":2},{"id":2,"name":"btn_c","params":3},{"id":3,"name":"btn_l","params":2},{"id":4,"name":"btn_r","params":2}],"dataVars":[{"name":"$display","kind":"string"}],"limits":{"instrPerTick":32,"usPerTick":300,"updateBudgetUs":3000}})json");
}

String BytecodeService::listJson() const {
  String json = "[";
  bool first = true;

  if (_packValid) {
    for (uint8_t i = 0; i < _packCount; i++) {
      PackEntry entry;
      if (!getPackEntry(_packView, i, entry)) continue;
      char name[kPackNameLen + 1];
      if (!packNameToText(entry.name, name)) continue;

      const int8_t ref = findRefByKey(entry.name);
      const uint8_t id = ref >= 0 ? _refs[ref].id : 0;
      const bool loaded = id != 0 && findSlot(id) >= 0;
      const bool active = id != 0 && _vm.isScriptActive(id);
      const bool error = id != 0 && _vm.lastError() != VmError::NONE &&
                         _vm.lastErrorScript() == id;

      if (!first) json += ',';
      first = false;
      json += "{\"name\":\"";
      json += name;
      json += "\",\"id\":";
      json += id;
      json += ",\"size\":";
      json += entry.codeLen;
      json += ",\"boot\":";
      json += strcmp(name, "boot") == 0 ? 1 : 0;
      json += ",\"loaded\":";
      json += loaded ? 1 : 0;
      json += ",\"active\":";
      json += active ? 1 : 0;
      json += ",\"error\":";
      json += error ? 1 : 0;
      json += ",\"flags\":";
      json += entry.flags;
      json += '}';
    }
  }

  json += ']';
  return json;
}

String BytecodeService::statusJson() const {
  char errorScriptName[kPackNameLen + 1];
  errorScriptName[0] = '\0';
  if (_vm.lastErrorScript() != 0) {
    const int8_t ref = findRefById(_vm.lastErrorScript());
    if (ref >= 0) packNameToText(_refs[ref].name, errorScriptName);
  }

  char buffer[384];
  snprintf(buffer, sizeof(buffer),
           "{\"active\":%u,\"scripts\":%u,\"saved\":%u,\"cacheSlots\":%u,"
           "\"packError\":%u,\"packErrorName\":\"%s\",\"error\":%u,"
           "\"errorName\":\"%s\",\"errorScript\":%u,\"errorScriptName\":\"%s\","
           "\"errorPc\":%u}",
           (unsigned)_vm.activeContexts(), (unsigned)_vm.scriptCount(),
           (unsigned)_packCount, (unsigned)kCacheSlots,
           (unsigned)_packError, packErrorName(_packError),
           (unsigned)_vm.lastError(), vmErrorName(_vm.lastError()),
           (unsigned)_vm.lastErrorScript(), errorScriptName,
           (unsigned)_vm.lastErrorPc());
  return String(buffer);
}

#endif  // ENABLE_BYTECODE_VM
