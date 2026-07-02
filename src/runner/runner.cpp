#include "./runner.h"

ScriptRunner* ScriptRunner::_instance = nullptr;

#define CHECK_SKIP() if (!_instance) return false; if (s.inIf && s.skipElse) return true;

uint32_t ScriptRunner::getUintVar(uint8_t idx) const {
  if (idx < MAX_UINT_VARS) return _uintVars[idx];
  return 0;
}

float ScriptRunner::getFloatVar(uint8_t idx) const {
  if (idx < MAX_FLOAT_VARS) return _floatVars[idx];
  return 0.0f;
}

void ScriptRunner::setUintVar(uint8_t idx, uint32_t value) {
  if (idx < MAX_UINT_VARS) _uintVars[idx] = value;
}

void ScriptRunner::setFloatVar(uint8_t idx, float value) {
  if (idx < MAX_FLOAT_VARS) _floatVars[idx] = value;
}

ScriptRunner::ScriptRunner(ScriptConflict defaultStrategy)
    : _defaultStrategy(defaultStrategy) {
  _instance = this;
  _activeCount = 0;
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) resetScriptState(i);
  for (int i = 0; i < MAX_UINT_VARS; i++) _uintVars[i] = 0;
  for (int i = 0; i < MAX_FLOAT_VARS; i++) _floatVars[i] = 0.0f;
  for (int i = 0; i < MAX_STRING_VARS; i++) _stringLen[i] = 0;
  _queueHead = _queueTail = _queueCount = 0;
  _registryCount = 0;
  _lastStateChangeTime = 0;
  _dataProvider = nullptr;
  _portProvider = nullptr;
  _stateChangeProvider = nullptr;
}

void ScriptRunner::resetScriptState(int idx) {
  ScriptState& s = _active[idx];
  s.active = false; s.id = 0; s.script = nullptr; s.pos = 0;
  s.inPause = false; s.pauseUntil = 0; s.startTime = 0; s.scriptLen = 0;
  s.inLoop = false; s.isInfinite = false; s.repeatCount = 0; s.loopStartPos = 0;
  s.lastExecutionTime = 0; s.inFade = false; s.fadeGpio = 0; s.fadeTarget = 0;
  s.fadeStartValue = 0; s.fadeStartTime = 0; s.fadeDuration = 0;
  s.inIf = false; s.ifResult = false; s.skipElse = false; s.ifDepth = 0;
  s.inWait = false; s.waitUntil = 0; s.inEvent = false; s.eventHash = 0;
}

void ScriptRunner::addToActiveList(uint8_t idx) {
  for (int i = 0; i < _activeCount; i++) if (_activeList[i] == idx) return;
  if (_activeCount < MAX_ACTIVE_SCRIPTS) _activeList[_activeCount++] = idx;
}

void ScriptRunner::removeFromActiveList(uint8_t idx) {
  for (int i = 0; i < _activeCount; i++) {
    if (_activeList[i] == idx) { _activeList[i] = _activeList[--_activeCount]; return; }
  }
}

void ScriptRunner::setDataProvider(DataProvider provider) { _dataProvider = provider; }
void ScriptRunner::setLogProvider(LogProvider provider) { _logProvider = provider; }
void ScriptRunner::setStateChangeProvider(StateChangeProvider provider) { _stateChangeProvider = provider; }
void ScriptRunner::setPortProvider(PortProvider provider) { _portProvider = provider; }

void ScriptRunner::addToRegistry(uint8_t id, const char* script) {
  int existing = findInRegistry(id);
  if (existing != -1) { _registry[existing].script = script; return; }
  if (_registryCount < MAX_REGISTERED_SCRIPTS) {
    _registry[_registryCount].id = id;
    _registry[_registryCount].script = script;
    _registryCount++;
  }
}

int ScriptRunner::findInRegistry(uint8_t id) const {
  for (int i = 0; i < _registryCount; i++) if (_registry[i].id == id) return i;
  return -1;
}

bool ScriptRunner::registerScript(uint8_t id, const char* script) {
  uint16_t len = strlen(script);
  if (len > 255) return false;
  addToRegistry(id, script);
  return true;
}

bool ScriptRunner::runScript(uint8_t id) {
  int regIdx = findInRegistry(id);
  if (regIdx == -1) return false;
  const char* script = _registry[regIdx].script;
  uint16_t len = strlen(script);
  int existingIdx = findById(id);
  if (existingIdx != -1) { activateSlot(existingIdx, id, script, len); return true; }
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active) { activateSlot(i, id, script, len); return true; }
  }
  return addToQueue(id, script, len);
}

bool ScriptRunner::addScript(uint8_t id, const char* script, ScriptConflict strategy) {
  uint16_t len = strlen(script);
  if (len > 255) return false;
  addToRegistry(id, script);
  int existingIdx = findById(id);
  if (existingIdx != -1) {
    switch (strategy) {
      case IGNORE: return true;
      case RESTART: activateSlot(existingIdx, id, script, len); return true;
      case RESTART_IF_SAME:
        if (strcmp(_active[existingIdx].script, script) == 0) { activateSlot(existingIdx, id, script, len); return true; }
        break;
      case ADD_QUEUE: return addToQueue(id, script, len);
    }
  }
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active) { activateSlot(i, id, script, len); return true; }
  }
  return addToQueue(id, script, len);
}

uint32_t ScriptRunner::hashEvent(const char* str) const {
  uint32_t hash = 5381;
  while (*str) hash = ((hash << 5) + hash) + *str++;
  return hash;
}

void ScriptRunner::emitEvent(uint32_t hash) {
  for (int i = 0; i < _activeCount; i++) {
    ScriptState& s = _active[_activeList[i]];
    if (s.inEvent && s.eventHash == hash) s.inEvent = false;
  }
}

void ScriptRunner::emitEvent(const char* eventName) { emitEvent(hashEvent(eventName)); }

bool ScriptRunner::handleOn(const char* token, ScriptState& s, uint32_t now) { CHECK_SKIP(); const char* p = token + 3; s.eventHash = _instance->hashEvent(p); s.inEvent = true; return true; }

bool ScriptRunner::handlePause(const char* token, ScriptState& s, uint32_t now) {
  CHECK_SKIP();
  const char* valStr = token + 1; bool isValid = true;
  for (int i = 0; valStr[i]; i++) if (!isdigit(valStr[i])) { isValid = false; break; }
  if (isValid) { uint8_t val = atoi(valStr); if (val > 0) { s.inPause = true; s.pauseUntil = now + (val * 100UL); } }
  return true;
}

bool ScriptRunner::handleWait(const char* token, ScriptState& s, uint32_t now) {
  CHECK_SKIP();
  const char* valStr = token + 5; uint32_t duration = 0, num = 0;
  while (isdigit(*valStr)) { num = num * 10 + (*valStr - '0'); valStr++; }
  char c = tolower(*valStr);
  if (c == 's') duration = num * 1000; else if (c == 'm') duration = num * 60000; else duration = num * 1000;
  if (duration > 0) { s.inWait = true; s.waitUntil = now + duration; return true; }
  return false;
}

bool ScriptRunner::handleLog(const char* token, ScriptState& s, uint32_t now) {
  CHECK_SKIP(); if (!_instance->_logProvider) return true;
  const char* msg = token + 4;
  if (strstr(msg, "$") == nullptr) { _instance->_logProvider(msg); return true; }
  char buffer[256]; const char* p = msg; int pos = 0;
  while (*p && pos < 250) {
    if (p[0] == '$') {
      p++; char type = p[0]; p++;
      if (type == 'v') { int idx = 0; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_UINT_VARS) pos += snprintf(buffer + pos, 250 - pos, "%u", _instance->_uintVars[idx]); }
      else if (type == 'f') { int idx = 0; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_FLOAT_VARS) pos += snprintf(buffer + pos, 250 - pos, "%.1f", _instance->_floatVars[idx]); }
      else if (type == 's') { int idx = 0; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_STRING_VARS) { if (p[0] == '#') { pos += snprintf(buffer + pos, 250 - pos, "%u", _instance->_stringLen[idx]); p++; } else for (int i = 0; i < _instance->_stringLen[idx] && pos < 250; i++) buffer[pos++] = _instance->_stringVars[idx][i]; } }
      else if (type == 'p') { int gpio = 0; while (isdigit(*p)) { gpio = gpio * 10 + (*p - '0'); p++; } uint16_t val = 0; if (_instance->_portProvider && _instance->_portProvider(gpio, PORT_READ, val)) pos += snprintf(buffer + pos, 250 - pos, "%u", val); }
      else if (type == 'a') { int pin = 0; while (isdigit(*p)) { pin = pin * 10 + (*p - '0'); p++; } uint16_t val = 0; if (_instance->_portProvider && _instance->_portProvider(pin, PORT_READ, val)) pos += snprintf(buffer + pos, 250 - pos, "%u", val); }
      else { const char* idStart = p - 2; while (*p && *p != ' ' && *p != ',' && *p != '$' && *p != '\0') p++; size_t idLen = p - idStart; char id[32]; if (idLen > 30) idLen = 30; strncpy(id, idStart, idLen); id[idLen] = '\0'; if (_instance->_dataProvider) { DataValue dv; if (_instance->_dataProvider(id, KIND_UINT, dv, false)) pos += snprintf(buffer + pos, 250 - pos, "%u", dv.uintVal); else { DataValue dv2; dv2.stringVal.data = (uint8_t*)(buffer + pos); dv2.stringVal.len = 250 - pos; if (_instance->_dataProvider(id, KIND_STRING, dv2, false)) pos += dv2.stringVal.len; } } }
    } else buffer[pos++] = *p++;
  }
  buffer[pos] = '\0'; _instance->_logProvider(buffer);
  return true;
}

bool ScriptRunner::handleIf(const char* token, ScriptState& s, uint32_t now) { CHECK_SKIP(); s.ifDepth++; s.inIf = true; s.skipElse = !_instance->parseCondition(token, s); return true; }

bool ScriptRunner::handleElse(const char* token, ScriptState& s, uint32_t now) {
  if (!_instance) return false;
  if (s.inIf && s.ifDepth > 0) {
    if (s.ifResult) s.skipElse = true;
    else { s.skipElse = false; s.ifResult = true; }
  }
  return true;
}

bool ScriptRunner::handleEnd(const char* token, ScriptState& s, uint32_t now) {
  if (!_instance) return false;
  if (s.ifDepth > 0) { s.ifDepth--; if (s.ifDepth == 0) { s.inIf = false; s.skipElse = false; s.ifResult = false; } }
  return true;
}

bool ScriptRunner::handleLoopStart(const char* token, ScriptState& s, uint32_t now) { CHECK_SKIP(); s.loopStartPos = s.pos; s.inLoop = true; if (_instance->isInfiniteLoop(token)) { s.isInfinite = true; s.repeatCount = 0; s.startTime = now; } else { s.isInfinite = false; s.repeatCount = _instance->parseLoopCount(token); if (s.repeatCount == 0) s.inLoop = false; } return true; }
bool ScriptRunner::handleLoopEnd(const char* token, ScriptState& s, uint32_t now) { CHECK_SKIP(); if (!s.inLoop) return true; if (s.isInfinite) { s.pos = s.loopStartPos; s.inPause = false; return true; } if (s.repeatCount > 0) { s.repeatCount--; if (s.repeatCount > 0) { s.pos = s.loopStartPos; s.inPause = false; return true; } } s.inLoop = false; s.isInfinite = false; s.repeatCount = 0; return true; }
bool ScriptRunner::handleCall(const char* token, ScriptState& s, uint32_t now) { CHECK_SKIP(); uint8_t targetId = atoi(token + 5); int regIdx = _instance->findInRegistry(targetId); if (regIdx != -1) { const char* script = _instance->_registry[regIdx].script; _instance->addScript(targetId, script, RESTART); return true; } return false; }

bool ScriptRunner::resolveValue(const char* p, uint32_t& value) {
  if (p[0] != '$') { value = atoi(p); return true; }
  char type = p[1];
  if (type == 'v') { int idx = 0; p += 2; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_UINT_VARS) { value = _instance->_uintVars[idx]; return true; } }
  else if (type == 'f') { int idx = 0; p += 2; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_FLOAT_VARS) { value = (uint32_t)_instance->_floatVars[idx]; return true; } }
  else if (type == 's') { int idx = 0; p += 2; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (p[0] == '#' && idx >= 0 && idx < MAX_STRING_VARS) { value = _instance->_stringLen[idx]; return true; } }
  else if (type == 'p') { int gpio = 0; p += 2; while (isdigit(*p)) { gpio = gpio * 10 + (*p - '0'); p++; } char id[16]; snprintf(id, 16, "$p%d", gpio); return _instance->getDataValue(id, value); }
  else if (type == 'a') { int pin = 0; p += 2; while (isdigit(*p)) { pin = pin * 10 + (*p - '0'); p++; } uint16_t val = 0; if (_instance->_portProvider && _instance->_portProvider(pin, PORT_READ, val)) { value = val; return true; } }
  else { const char* idStart = p - 1; const char* idEnd = p; while (*idEnd && *idEnd != ',' && *idEnd != ' ' && *idEnd != ')' && *idEnd != '&' && *idEnd != '|') idEnd++; size_t idLen = idEnd - idStart; char id[32]; if (idLen > 30) idLen = 30; strncpy(id, idStart, idLen); id[idLen] = '\0'; if (_instance->_dataProvider) { DataValue dv; if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { value = dv.uintVal; return true; } } }
  return false;
}

static uint8_t hexCharToByte(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  return 0;
}

bool ScriptRunner::handleVariable(const char* token, ScriptState& s, uint32_t now) {
  CHECK_SKIP();
  const char* p = token + 1;
  char type = p[0]; p++;

  if (type == 'p') {
    int gpio = 0; while (isdigit(*p)) { gpio = gpio * 10 + (*p - '0'); p++; }
    if (*p != '=') return true; p++;
    const char* valStart = p; int slashPos = -1;
    for (int i = 0; valStart[i]; i++) if (valStart[i] == '/') { slashPos = i; break; }
    uint16_t value = atoi(valStart), duration = 0;
    if (slashPos != -1) duration = atoi(valStart + slashPos + 1);
    uint32_t currentValue = 0; char id[16]; snprintf(id, 16, "$p%d", gpio);
    _instance->getDataValue(id, currentValue);
    if (duration > 0) _instance->startFade(s, gpio, value, duration, (uint16_t)currentValue);
    else _instance->setOutput(gpio, value, false);
    return true;
  }

  if (type == 'v' || type == 'f') {
    bool isFloat = (type == 'f');
    int idx = 0; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; }
    int maxVars = isFloat ? MAX_FLOAT_VARS : MAX_UINT_VARS;
    if (idx < 0 || idx >= maxVars || *p != '=') return true;
    p++;

    if (p[0] == '&') { if (!isFloat) _instance->_uintVars[idx] &= atoi(p + 1); }
    else if (p[0] == '|') { if (!isFloat) _instance->_uintVars[idx] |= atoi(p + 1); }
    else if (p[0] == '^') { if (!isFloat) _instance->_uintVars[idx] ^= atoi(p + 1); }
    else if (p[0] == '~') { if (!isFloat) _instance->_uintVars[idx] = ~_instance->_uintVars[idx]; }
    else if (p[0] == '<' && p[1] == '<') { if (!isFloat) _instance->_uintVars[idx] <<= atoi(p + 2); }
    else if (p[0] == '>' && p[1] == '>') { if (!isFloat) _instance->_uintVars[idx] >>= atoi(p + 2); }
    else if (p[0] == '%') { if (!isFloat) { int d = atoi(p + 1); if (d != 0) _instance->_uintVars[idx] %= d; } else {} }
    else if (p[0] == '@') { if (!isFloat) _instance->_uintVars[idx] = random(atoi(p + 1)); else {} }
    else if (p[0] == '+') { if (isFloat) _instance->_floatVars[idx] += atof(p + 1); else _instance->_uintVars[idx] += atoi(p + 1); }
    else if (p[0] == '-') { if (isFloat) _instance->_floatVars[idx] -= atof(p + 1); else _instance->_uintVars[idx] -= atoi(p + 1); }
    else if (p[0] == '*') { if (isFloat) _instance->_floatVars[idx] *= atof(p + 1); else _instance->_uintVars[idx] *= atoi(p + 1); }
    else if (p[0] == '/') { if (isFloat) { float d = atof(p + 1); if (d != 0) _instance->_floatVars[idx] /= d; } else { int d = atoi(p + 1); if (d != 0) _instance->_uintVars[idx] /= d; } }
    else if (p[0] == '$') { uint32_t val = 0; if (_instance->resolveValue(p, val)) { if (isFloat) _instance->_floatVars[idx] = (float)val; else _instance->_uintVars[idx] = val; } }
    else { if (isFloat) _instance->_floatVars[idx] = atof(p); else _instance->_uintVars[idx] = atoi(p); }
    return true;
  }

  if (type == 'u') {
    int idx = 0; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; }
    if (*p != '=') return true; p++;
    if (p[0] == '$' && p[1] == 's') { int srcIdx = 0; p += 2; while (isdigit(*p)) { srcIdx = srcIdx * 10 + (*p - '0'); p++; } if (srcIdx >= 0 && srcIdx < MAX_STRING_VARS && _instance->_dataProvider) { char id[8]; snprintf(id, 8, "$u%d", idx); DataValue dv; dv.stringVal.data = _instance->_stringVars[srcIdx]; dv.stringVal.len = _instance->_stringLen[srcIdx]; _instance->_dataProvider(id, KIND_STRING, dv, true); } }
    return true;
  }

  if (type == 's') {
    int idx = 0; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; }
    if (idx < 0 || idx >= MAX_STRING_VARS) return true;
    if (p[0] == '{') { p++; int byteIdx = 0; while (isdigit(*p)) { byteIdx = byteIdx * 10 + (*p - '0'); p++; } if (p[0] == ':') { p++; int endIdx = 0; while (isdigit(*p)) { endIdx = endIdx * 10 + (*p - '0'); p++; } if (p[0] == '}') p++; return true; } if (p[0] == '}') p++; if (p[0] == '=') { p++; uint8_t val = atoi(p); if (byteIdx >= 0 && byteIdx < MAX_STRING_LEN) { _instance->_stringVars[idx][byteIdx] = val; if (byteIdx >= _instance->_stringLen[idx]) _instance->_stringLen[idx] = byteIdx + 1; } } return true; }
    if (p[0] == '#') return true;
    if (p[0] != '=' && p[0] != '+') return true;
    bool append = (p[0] == '+'); p++;
    if (!append && (*p == '\0' || *p == ',')) { _instance->_stringLen[idx] = 0; return true; }
    if (!append && p[0] == '$' && p[1] == 's') { int srcIdx = 0; p += 2; while (isdigit(*p)) { srcIdx = srcIdx * 10 + (*p - '0'); p++; } if (srcIdx >= 0 && srcIdx < MAX_STRING_VARS) { uint8_t len = _instance->_stringLen[srcIdx]; if (len > MAX_STRING_LEN) len = MAX_STRING_LEN; memcpy(_instance->_stringVars[idx], _instance->_stringVars[srcIdx], len); _instance->_stringLen[idx] = len; } return true; }
    if (!append && p[0] == '$' && p[1] == 'u') { int uartNum = 0; p += 2; while (isdigit(*p)) { uartNum = uartNum * 10 + (*p - '0'); p++; } char id[8]; snprintf(id, 8, "$u%d", uartNum); if (_instance->_dataProvider) { DataValue dv; dv.stringVal.data = _instance->_stringVars[idx]; dv.stringVal.len = MAX_STRING_LEN; if (_instance->_dataProvider(id, KIND_STRING, dv, false)) _instance->_stringLen[idx] = dv.stringVal.len; else _instance->_stringLen[idx] = 0; } return true; }
    if (append) { if (p[0] == 'h') { p++; while (*p && *p != ',' && *p != ']') { if (isxdigit(p[0]) && isxdigit(p[1])) { uint8_t b = (hexCharToByte(p[0]) << 4) | hexCharToByte(p[1]); if (_instance->_stringLen[idx] < MAX_STRING_LEN) _instance->_stringVars[idx][_instance->_stringLen[idx]++] = b; p += 2; } else p++; } } else while (*p && *p != ',' && *p != ']') { if (_instance->_stringLen[idx] < MAX_STRING_LEN) _instance->_stringVars[idx][_instance->_stringLen[idx]++] = *p; p++; } return true; }
    if (p[0] == 'h') { p++; _instance->_stringLen[idx] = 0; while (*p && *p != ',' && *p != ']') { if (isxdigit(p[0]) && isxdigit(p[1])) { uint8_t b = (hexCharToByte(p[0]) << 4) | hexCharToByte(p[1]); if (_instance->_stringLen[idx] < MAX_STRING_LEN) _instance->_stringVars[idx][_instance->_stringLen[idx]++] = b; p += 2; } else p++; } return true; }
    _instance->_stringLen[idx] = 0; while (*p && *p != ',' && *p != ']') { if (_instance->_stringLen[idx] < MAX_STRING_LEN) _instance->_stringVars[idx][_instance->_stringLen[idx]++] = *p; p++; }
    return true;
  }

  if (type == 'a') {
    int pin = 0; while (isdigit(*p)) { pin = pin * 10 + (*p - '0'); p++; }
    if (*p != '=') return true; p++;
    uint16_t val = 0;
    if (_instance->_portProvider && _instance->_portProvider(pin, PORT_READ, val)) {
      if (p[0] == '$' && p[1] == 'v') { int idx = 0; p += 2; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_UINT_VARS) _instance->_uintVars[idx] = val; }
      else if (p[0] == '$' && p[1] == 'f') { int idx = 0; p += 2; while (isdigit(*p)) { idx = idx * 10 + (*p - '0'); p++; } if (idx >= 0 && idx < MAX_FLOAT_VARS) _instance->_floatVars[idx] = (float)val; }
    }
    return true;
  }

  // Внешняя переменная
  {
    const char* idStart = p - 2; while (*p && *p != '=') p++;
    if (*p != '=') return true;
    size_t idLen = p - idStart; char id[32]; if (idLen > 30) idLen = 30;
    strncpy(id, idStart, idLen); id[idLen] = '\0'; p++;
    if (_instance->_dataProvider) {
      DataValue dv;
      if (p[0] == '&') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal &= atoi(p + 1); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '|') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal |= atoi(p + 1); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '^') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal ^= atoi(p + 1); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '~') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal = ~dv.uintVal; _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '<' && p[1] == '<') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal <<= atoi(p + 2); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '>' && p[1] == '>') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal >>= atoi(p + 2); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '%') { int d = atoi(p + 1); if (d != 0 && _instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal %= d; _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '@') { dv.uintVal = random(atoi(p + 1)); _instance->_dataProvider(id, KIND_UINT, dv, true); }
      else if (p[0] == '+') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal += atoi(p + 1); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '-') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal -= atoi(p + 1); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '*') { if (_instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal *= atoi(p + 1); _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else if (p[0] == '/') { int d = atoi(p + 1); if (d != 0 && _instance->_dataProvider(id, KIND_UINT, dv, false)) { dv.uintVal /= d; _instance->_dataProvider(id, KIND_UINT, dv, true); } }
      else { dv.uintVal = atoi(p); _instance->_dataProvider(id, KIND_UINT, dv, true); }
    }
    return true;
  }
  return true;
}

static bool isFastCommand(const char* token) {
  if (token[0] != '$') return false;
  char type = token[1];
  return (type == 'v' || type == 'f' || type == 's');
}

bool ScriptRunner::checkScriptState(ScriptState& s, uint32_t now) {
  if (s.inEvent || s.inPause || s.inWait || s.inFade) {
    if (s.inPause) { if (now >= s.pauseUntil) s.inPause = false; else return false; }
    if (s.inWait) { if (now >= s.waitUntil) s.inWait = false; else return false; }
    if (s.inFade) { updateFade(s, now); if (s.inFade) return false; }
  }
  return true;
}

bool ScriptRunner::tryLoopContinue(ScriptState& s) {
  if (!s.inLoop) return false;
  if (s.isInfinite) { s.pos = s.loopStartPos; s.inPause = false; return true; }
  if (s.repeatCount > 0) { s.repeatCount--; if (s.repeatCount > 0) { s.pos = s.loopStartPos; s.inPause = false; return true; } }
  s.inLoop = false; s.isInfinite = false; s.repeatCount = 0; return false;
}

void ScriptRunner::update() {
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active && _queueCount > 0) { uint8_t id = _queueId[_queueHead]; const char* script = _queueScript[_queueHead]; uint16_t len = _queueLen[_queueHead]; _queueHead = (_queueHead + 1) % QUEUE_SIZE; _queueCount--; activateSlot(i, id, script, len); }
  }
  for (int i = 0; i < _activeCount; i++) {
    uint8_t idx = _activeList[i]; ScriptState& s = _active[idx];
    if (!s.active) { removeFromActiveList(idx); i--; continue; }
    uint32_t now = millis();
    if (!s.isInfinite) { if (now - s.startTime > 60000) { s.active = false; removeFromActiveList(idx); i--; continue; } } else s.startTime = now;

    if (s.pos < s.scriptLen) {
      const char* p = s.script + s.pos;
      while (*p == ' ' || *p == ',') { p++; if (p - s.script >= s.scriptLen) break; }
      if (p - s.script < s.scriptLen && *p != '\0') {
        const char* start = p;
        while (*p && *p != ',' && *p != ']' && (p - s.script) < s.scriptLen) p++;
        size_t len = p - start;
        if (len > 0 && len < MAX_TOKEN_LEN) {
          static char previewToken[MAX_TOKEN_LEN];
          strncpy(previewToken, start, len); previewToken[len] = '\0';
          if (!isFastCommand(previewToken) && !(s.inIf && s.skipElse)) {
            if (now - s.lastExecutionTime < 100) continue;
            s.lastExecutionTime = now;
          }
        }
      }
    } else {
      if (!(s.inIf && s.skipElse)) {
        if (now - s.lastExecutionTime < 100) continue;
        s.lastExecutionTime = now;
      }
    }

    if (!checkScriptState(s, now)) continue;
    if (s.pos >= s.scriptLen) { if (tryLoopContinue(s)) continue; s.active = false; removeFromActiveList(idx); i--; continue; }

    const char* p = s.script + s.pos;
    while (*p == ' ' || *p == ',') { p++; s.pos++; if (s.pos >= s.scriptLen) { if (tryLoopContinue(s)) continue; s.active = false; removeFromActiveList(idx); i--; return; } }
    if (*p == '\0' || s.pos >= s.scriptLen) { if (tryLoopContinue(s)) continue; s.active = false; removeFromActiveList(idx); i--; continue; }

    const char* start = p;
    if (*p == '[') { p++; while (*p && *p != ']' && (p - s.script) < s.scriptLen) p++; if (*p == ']') p++; }
    else if (*p == ']') p++;
    else while (*p && *p != ',' && *p != ']' && (p - s.script) < s.scriptLen) p++;

    size_t len = p - start;
    if (len >= MAX_TOKEN_LEN) { s.pos = p - s.script; continue; }

    static char token[MAX_TOKEN_LEN]; strncpy(token, start, len); token[len] = '\0'; s.pos = p - s.script;
    processToken(token, s, now);
  }
}

bool ScriptRunner::stopScript(uint8_t id) { int idx = findById(id); if (idx == -1) return false; _active[idx].active = false; removeFromActiveList(idx); return true; }
void ScriptRunner::stopAll() { for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) _active[i].active = false; _activeCount = 0; _queueCount = _queueHead = _queueTail = 0; }
bool ScriptRunner::isRunning(uint8_t id) const { return findById(id) != -1; }
bool ScriptRunner::isBusy() const { return _activeCount > 0 || _queueCount > 0; }
int ScriptRunner::findById(uint8_t id) const { for (int i = 0; i < _activeCount; i++) { uint8_t idx = _activeList[i]; if (_active[idx].active && _active[idx].id == id) return idx; } return -1; }
bool ScriptRunner::addToQueue(uint8_t id, const char* script, uint16_t len) { if (_queueCount >= QUEUE_SIZE) return false; _queueId[_queueTail] = id; _queueScript[_queueTail] = script; _queueLen[_queueTail] = len; _queueTail = (_queueTail + 1) % QUEUE_SIZE; _queueCount++; return true; }
void ScriptRunner::activateSlot(int idx, uint8_t id, const char* script, uint16_t len) { resetScriptState(idx); _active[idx].active = true; _active[idx].id = id; _active[idx].script = script; _active[idx].scriptLen = len; _active[idx].startTime = millis(); addToActiveList(idx); }

bool ScriptRunner::getDataValue(const char* id, uint32_t& value) {
  if (id[0] == '$' && id[1] == 'p') { int gpio = atoi(id + 2); uint16_t val = 0; if (_portProvider && _portProvider(gpio, PORT_READ, val)) { value = val; return true; } return false; }
  if (id[0] == '$' && id[1] == 'a') { int pin = atoi(id + 2); uint16_t val = 0; if (_portProvider && _portProvider(pin, PORT_READ, val)) { value = val; return true; } return false; }
  if (id[0] == '$' && id[1] == 'v') { int idx = atoi(id + 2); if (idx >= 0 && idx < MAX_UINT_VARS) { value = _uintVars[idx]; return true; } return false; }
  if (id[0] == '$' && id[1] == 'f') { int idx = atoi(id + 2); if (idx >= 0 && idx < MAX_FLOAT_VARS) { value = (uint32_t)_floatVars[idx]; return true; } return false; }
  if (_dataProvider) { DataValue dv; if (_dataProvider(id, KIND_UINT, dv, false)) { value = dv.uintVal; return true; } }
  return false;
}

uint8_t ScriptRunner::parseLoopCount(const char* token) { if (token[0] != '[') return 0; size_t len = strlen(token); if (len < 3 || token[len - 1] != ']') return 0; char numStr[4]; strncpy(numStr, token + 1, len - 2); numStr[len - 2] = '\0'; return atoi(numStr); }
bool ScriptRunner::isInfiniteLoop(const char* token) { return strcmp(token, "[*]") == 0; }

bool ScriptRunner::parseCondition(const char* token, ScriptState& s) {
  const char* p = token + 3; bool finalResult = false, firstCondition = true; char pendingOp = '\0';
  while (*p && *p != ',') {
    const char* idStart = p, *opStart = nullptr, *opEnd = nullptr;
    while (*p && *p != ',' && *p != '&' && *p != '|') { if (p[0] == '>' && p[1] == '=') { opStart = p; opEnd = p + 2; break; } else if (p[0] == '<' && p[1] == '=') { opStart = p; opEnd = p + 2; break; } else if (p[0] == '=' && p[1] == '=') { opStart = p; opEnd = p + 2; break; } else if (p[0] == '!' && p[1] == '=') { opStart = p; opEnd = p + 2; break; } else if (p[0] == '>' || p[0] == '<') { opStart = p; opEnd = p + 1; break; } p++; }
    if (!opStart) { s.ifResult = false; return false; }
    char id[32]; size_t idLen = opStart - idStart; if (idLen >= 31) idLen = 30; strncpy(id, idStart, idLen); id[idLen] = '\0';
    char op1 = opStart[0], op2 = (opEnd - opStart == 2) ? opStart[1] : '\0';
    bool isFloat = (id[0] == '$' && id[1] == 'f');
    bool isString = (id[0] == '$' && id[1] == 's');
    uint32_t currentValue;
    bool condResult;

    if (isString) {
      int idx = atoi(id + 2);
      if (idx >= 0 && idx < MAX_STRING_VARS) {
        size_t cmpLen = strlen(opEnd);
        bool match = (cmpLen == _stringLen[idx] && memcmp(_stringVars[idx], opEnd, cmpLen) == 0);
        if (op1 == '=' && op2 == '=') condResult = match;
        else if (op1 == '!' && op2 == '=') condResult = !match;
        else condResult = false;
      } else condResult = false;
    } else if (isFloat) {
      if (!getDataValue(id, currentValue)) { s.ifResult = false; return false; }
      float cf = _floatVars[atoi(id + 2)], comp = atof(opEnd);
      if (op1 == '>' && op2 == '=') condResult = cf >= comp;
      else if (op1 == '<' && op2 == '=') condResult = cf <= comp;
      else if (op1 == '=' && op2 == '=') condResult = cf == comp;
      else if (op1 == '!' && op2 == '=') condResult = cf != comp;
      else if (op1 == '>') condResult = cf > comp;
      else if (op1 == '<') condResult = cf < comp;
      else condResult = false;
    } else {
      if (!getDataValue(id, currentValue)) { s.ifResult = false; return false; }
      uint32_t cv = atoi(opEnd);
      if (op1 == '>' && op2 == '=') condResult = currentValue >= cv;
      else if (op1 == '<' && op2 == '=') condResult = currentValue <= cv;
      else if (op1 == '=' && op2 == '=') condResult = currentValue == cv;
      else if (op1 == '!' && op2 == '=') condResult = currentValue != cv;
      else if (op1 == '>') condResult = currentValue > cv;
      else if (op1 == '<') condResult = currentValue < cv;
      else condResult = false;
    }
    while (*p && *p != ',' && *p != '&' && *p != '|') p++;
    if (firstCondition) { finalResult = condResult; firstCondition = false; } else { if (pendingOp == '&') finalResult = finalResult && condResult; else if (pendingOp == '|') finalResult = finalResult || condResult; }
    if (p[0] == '&' && p[1] == '&') { pendingOp = '&'; p += 2; } else if (p[0] == '|' && p[1] == '|') { pendingOp = '|'; p += 2; } else break;
  }
  s.ifResult = finalResult; return finalResult;
}

void ScriptRunner::startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, uint16_t startValue) { if (duration == 0) { setOutput(gpio, target, false); return; } s.inFade = true; s.fadeGpio = gpio; s.fadeTarget = target; s.fadeStartValue = startValue; s.fadeStartTime = millis(); s.fadeDuration = duration * 100UL; }
void ScriptRunner::updateFade(ScriptState& s, uint32_t now) { if (!s.inFade) return; uint32_t elapsed = now - s.fadeStartTime; if (elapsed >= s.fadeDuration) { setOutput(s.fadeGpio, s.fadeTarget, false); s.inFade = false; } else { int16_t delta = s.fadeTarget - s.fadeStartValue; uint16_t currentValue; if (delta >= 0) currentValue = s.fadeStartValue + (uint16_t)((uint32_t)delta * elapsed / s.fadeDuration); else currentValue = s.fadeStartValue - (uint16_t)((uint32_t)(-delta) * elapsed / s.fadeDuration); setOutput(s.fadeGpio, currentValue, true); } }
void ScriptRunner::setOutput(uint8_t gpio, uint16_t value, bool isFadeStep) { if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE; if (_portProvider) _portProvider(gpio, PORT_WRITE, value); if (!isFadeStep && _stateChangeProvider) { uint32_t now = millis(); if (now - _lastStateChangeTime >= 100) { _lastStateChangeTime = now; _stateChangeProvider(gpio, 0, value); } } }

bool ScriptRunner::processToken(const char* token, ScriptState& s, uint32_t now) {
  if (!_instance) return false;
  switch (token[0]) {
    case '[': return handleLoopStart(token, s, now); case ']': return handleLoopEnd(token, s, now);
    case 'c': if (strncmp(token, "call:", 5) == 0) return handleCall(token, s, now); break;
    case 'i': if (strncmp(token, "if:", 3) == 0) return handleIf(token, s, now); break;
    case 'e': if (strcmp(token, "else") == 0) return handleElse(token, s, now); if (strcmp(token, "end") == 0) return handleEnd(token, s, now); break;
    case 'l': if (strncmp(token, "log:", 4) == 0) return handleLog(token, s, now); break;
    case 'o': if (strncmp(token, "on:", 3) == 0) return handleOn(token, s, now); break;
    case 'w': if (strncmp(token, "wait:", 5) == 0) return handleWait(token, s, now); break;
    case 'p': case 'P': if (isdigit(token[1])) return handlePause(token, s, now); break;
    case '$': return handleVariable(token, s, now);
  }
  return false;
}