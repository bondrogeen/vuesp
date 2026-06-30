#include "./include/runner.h"

ScriptRunner* ScriptRunner::_instance = nullptr;

const TokenHandler ScriptRunner::_handlers[] = {
    {"[", 1, isLoopStart, handleLoopStart},
    {"]", 1, isLoopEnd, handleLoopEnd},
    {"if:", 5, isIfStart, handleIf},
    {"else", 5, isElse, handleElse},
    {"end", 5, isEnd, handleEnd},
    {"log:", 10, isLog, handleLog},
    {"wait:", 10, isWait, handleWait},
    {"p", 10, isPause, handlePause},
    {"$", 20, isVariable, handleVariable},
    {NULL, 255, isGPIO, handleGPIO},
};

bool ScriptRunner::isPause(const char* token, ScriptState& s) {
  return (token[0] == 'p' || token[0] == 'P') && isdigit(token[1]);
}

bool ScriptRunner::isWait(const char* token, ScriptState& s) {
  return strncmp(token, "wait:", 5) == 0;
}

bool ScriptRunner::isLog(const char* token, ScriptState& s) {
  return strncmp(token, "log:", 4) == 0;
}

bool ScriptRunner::isIfStart(const char* token, ScriptState& s) {
  return strncmp(token, "if:", 3) == 0;
}

bool ScriptRunner::isElse(const char* token, ScriptState& s) {
  return strcmp(token, "else") == 0;
}

bool ScriptRunner::isEnd(const char* token, ScriptState& s) {
  return strcmp(token, "end") == 0;
}

bool ScriptRunner::isLoopStart(const char* token, ScriptState& s) {
  if (token[0] != '[') return false;
  size_t len = strlen(token);
  if (len < 2) return false;
  return token[len - 1] == ']';
}

bool ScriptRunner::isLoopEnd(const char* token, ScriptState& s) {
  if (strcmp(token, "]") == 0) return true;
  size_t len = strlen(token);
  if (len > 0 && token[len - 1] == ']') {
    return true;
  }
  return false;
}

bool ScriptRunner::isVariable(const char* token, ScriptState& s) {
  return token[0] == '$';
}

bool ScriptRunner::isGPIO(const char* token, ScriptState& s) {
  int colonPos = -1;
  for (int i = 0; token[i]; i++) {
    if (token[i] == ':') {
      colonPos = i;
      break;
    }
  }
  if (colonPos == -1) return false;
  for (int i = 0; i < colonPos; i++) {
    if (!isdigit(token[i])) return false;
  }
  return true;
}

uint32_t ScriptRunner::getUintVar(uint8_t idx) const {
  if (idx < MAX_UINT_VARS) return _uintVars[idx];
  return 0;
}

float ScriptRunner::getFloatVar(uint8_t idx) const {
  if (idx < MAX_FLOAT_VARS) return _floatVars[idx];
  return 0.0f;
}

ScriptRunner::ScriptRunner(ScriptConflict defaultStrategy)
    : _defaultStrategy(defaultStrategy) {
  Serial.println("🔧 ScriptRunner constructor");
  _instance = this;

  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    _active[i].active = false;
    _active[i].id = 0;
    _active[i].script = nullptr;
    _active[i].pos = 0;
    _active[i].inPause = false;
    _active[i].pauseUntil = 0;
    _active[i].startTime = 0;
    _active[i].scriptLen = 0;
    _active[i].inLoop = false;
    _active[i].isInfinite = false;
    _active[i].repeatCount = 0;
    _active[i].loopStartPos = 0;
    _active[i].lastExecutionTime = 0;

    _active[i].inFade = false;
    _active[i].fadeGpio = 0;
    _active[i].fadeTarget = 0;
    _active[i].fadeStartValue = 0;
    _active[i].fadeStartTime = 0;
    _active[i].fadeDuration = 0;
    _active[i].fadeIsPwm = false;

    _active[i].inIf = false;
    _active[i].ifResult = false;
    _active[i].skipElse = false;
    _active[i].ifDepth = 0;

    _active[i].inWait = false;
    _active[i].waitUntil = 0;
  }
  for (int i = 0; i < MAX_UINT_VARS; i++) {
    _uintVars[i] = 0;
  }
  for (int i = 0; i < MAX_FLOAT_VARS; i++) {
    _floatVars[i] = 0.0f;
  }
  _queueHead = 0;
  _queueTail = 0;
  _queueCount = 0;
  _dataSourcesCount = 0;
  _lastStateChangeTime = 0;

  _readProvider = nullptr;
  _writeProvider = nullptr;
  _portOutputCallback = nullptr;
  _stateChangeProvider = nullptr;
  Serial.println("✅ ScriptRunner constructor done");
}

void ScriptRunner::setReadProvider(ReadProvider provider) {
  Serial.println("🔧 setReadProvider");
  _readProvider = provider;
}

void ScriptRunner::setWriteProvider(WriteProvider provider) {
  Serial.println("🔧 setWriteProvider");
  _writeProvider = provider;
}

void ScriptRunner::setLogProvider(LogProvider provider) {
  Serial.println("🔧 setLogProvider");
  _logProvider = provider;
}

void ScriptRunner::setStateChangeProvider(StateChangeProvider provider) {
  Serial.println("🔧 setStateChangeProvider");
  _stateChangeProvider = provider;
}

void ScriptRunner::setPortOutputCallback(PortOutputCallback callback) {
  Serial.println("🔧 setPortOutputCallback");
  _portOutputCallback = callback;
}

void ScriptRunner::addDataSource(const char* id, DataType type, void* ptr) {
  if (_dataSourcesCount >= MAX_DATA_SOURCES) return;

  strncpy(_dataSources[_dataSourcesCount].id, id, 15);
  _dataSources[_dataSourcesCount].id[15] = '\0';
  _dataSources[_dataSourcesCount].type = type;
  _dataSources[_dataSourcesCount].ptr = ptr;
  _dataSourcesCount++;
}

bool ScriptRunner::addScript(uint8_t id, const char* script, ScriptConflict strategy) {
  Serial.print("📝 addScript ID:");
  Serial.print(id);
  Serial.print(" script: ");
  Serial.println(script);

  uint16_t len = strlen(script);
  if (len > 255) {
    Serial.println("❌ Script too long!");
    return false;
  }

  int existingIdx = findById(id);

  if (existingIdx != -1) {
    Serial.print("   Script ID ");
    Serial.print(id);
    Serial.println(" already exists");
    
    switch (strategy) {
      case IGNORE:
        Serial.println("   IGNORE");
        return true;
      case RESTART:
        Serial.println("   RESTART");
        activateSlot(existingIdx, id, script, len);
        return true;
      case RESTART_IF_SAME:
        if (strcmp(_active[existingIdx].script, script) == 0) {
          Serial.println("   RESTART_IF_SAME: same script, restarting");
          activateSlot(existingIdx, id, script, len);
          return true;
        }
        break;
      case ADD_QUEUE:
        Serial.println("   ADD_QUEUE");
        return addToQueue(id, script, len);
    }
  }

  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active) {
      Serial.print("   Activating slot ");
      Serial.println(i);
      activateSlot(i, id, script, len);
      return true;
    }
  }

  Serial.println("   No free slots, adding to queue");
  return addToQueue(id, script, len);
}

bool ScriptRunner::handlePause(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("⏸️ handlePause: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.inIf && s.skipElse) return true;

  const char* valStr = token + 1;
  bool isValid = true;
  for (int i = 0; valStr[i]; i++) {
    if (!isdigit(valStr[i])) {
      isValid = false;
      break;
    }
  }
  if (isValid) {
    uint8_t val = atoi(valStr);
    if (val > 0) {
      s.inPause = true;
      s.pauseUntil = now + (val * 100UL);
    }
  }
  return true;
}

bool ScriptRunner::handleWait(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("⏳ handleWait: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.inIf && s.skipElse) return true;

  const char* valStr = token + 5;
  uint32_t duration = 0;

  uint32_t num = 0;
  while (isdigit(*valStr)) {
    num = num * 10 + (*valStr - '0');
    valStr++;
  }

  char c = tolower(*valStr);
  if (c == 's') {
    duration = num * 1000;
  } else if (c == 'm') {
    duration = num * 60000;
  } else {
    duration = num * 1000;
  }

  if (duration > 0) {
    s.inWait = true;
    s.waitUntil = now + duration;
    return true;
  }

  return false;
}

bool ScriptRunner::handleLog(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("📝 handleLog: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.inIf && s.skipElse) return true;
  if (_instance->_logProvider) {
    const char* msg = token + 4;
    if (strstr(msg, "$") != nullptr) {
      char buffer[256];
      const char* p = msg;
      int pos = 0;

      while (*p && pos < 250) {
        if (p[0] == '$') {
          p++;
          char type = p[0];
          p++;

          if (type == 'u') {
            int idx = p[0] - '0';
            if (idx >= 0 && idx < MAX_UINT_VARS && p[1] == '\0') {
              pos += snprintf(buffer + pos, 250 - pos, "%u", _instance->_uintVars[idx]);
              p++;
            }
          } else if (type == 'f') {
            int idx = p[0] - '0';
            if (idx >= 0 && idx < MAX_FLOAT_VARS && p[1] == '\0') {
              pos += snprintf(buffer + pos, 250 - pos, "%.1f", _instance->_floatVars[idx]);
              p++;
            }
          } else if (type == 'p') {
            char id[16];
            snprintf(id, 16, "$%s", p - 1);
            uint32_t value;
            DataType dt;
            if (_instance->_readProvider && _instance->_readProvider(id, dt, value)) {
              pos += snprintf(buffer + pos, 250 - pos, "%u", value);
            }
            while (isdigit(*p)) p++;
          } else if (type == 'a') {
            int pin = p[0] - '0';
            if (p[1] == '\0') {
              pos += snprintf(buffer + pos, 250 - pos, "%u", analogRead(pin));
              p++;
            }
          } else if (type == 'd') {
            // ИСПРАВЛЕНО: удалена неиспользуемая переменная addr
            char id[32];
            snprintf(id, 32, "$%s", p - 1);
            uint32_t value;
            DataType dt;
            if (_instance->_readProvider && _instance->_readProvider(id, dt, value)) {
              pos += snprintf(buffer + pos, 250 - pos, "%.1f", (float)value / 10.0f);
            }
            while (*p && *p != ' ' && *p != ',' && *p != '\0') p++;
          }
        } else {
          buffer[pos++] = *p;
          p++;
        }
      }
      buffer[pos] = '\0';
      _instance->_logProvider(buffer);
    } else {
      _instance->_logProvider(msg);
    }
  }
  return true;
}

bool ScriptRunner::handleIf(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔀 handleIf: ");
  Serial.println(token);
  if (!_instance) return false;
  
  s.ifDepth++;
  s.inIf = true;

  if (_instance->parseCondition(token, s)) {
    s.skipElse = false;
    Serial.println("   ✅ Condition TRUE");
  } else {
    s.skipElse = true;
    Serial.println("   ❌ Condition FALSE");
  }
  return true;
}

bool ScriptRunner::handleElse(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔀 handleElse: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.inIf && s.ifDepth > 0) {
    if (s.ifResult) {
      s.skipElse = true;
    } else {
      s.skipElse = false;
      s.ifResult = true;
    }
  }
  return true;
}

bool ScriptRunner::handleEnd(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔀 handleEnd: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.ifDepth > 0) {
    s.ifDepth--;
    if (s.ifDepth == 0) {
      s.inIf = false;
      s.skipElse = false;
      s.ifResult = false;
    }
  }
  return true;
}

bool ScriptRunner::handleLoopStart(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔄 handleLoopStart: ");
  Serial.println(token);
  if (!_instance) return false;
  s.loopStartPos = s.pos;
  s.inLoop = true;

  if (_instance->isInfiniteLoop(token)) {
    s.isInfinite = true;
    s.repeatCount = 0;
    s.startTime = now;
  } else {
    s.isInfinite = false;
    s.repeatCount = _instance->parseLoopCount(token);
    if (s.repeatCount == 0) {
      s.inLoop = false;
    }
  }
  return true;
}

bool ScriptRunner::handleLoopEnd(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔄 handleLoopEnd: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.inLoop) {
    if (s.isInfinite) {
      s.pos = s.loopStartPos;
      s.inPause = false;
      return true;
    } else if (s.repeatCount > 0) {
      s.repeatCount--;
      if (s.repeatCount > 0) {
        s.pos = s.loopStartPos;
        s.inPause = false;
        return true;
      }
    }
    s.inLoop = false;
    s.isInfinite = false;
    s.repeatCount = 0;
  }
  return true;
}

bool ScriptRunner::handleVariable(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔧 handleVariable: ");
  Serial.println(token);
  
  if (!_instance) return false;
  if (s.inIf && s.skipElse) {
    Serial.println("   ⏭️ SKIPPED (skipElse)");
    return true;
  }

  const char* p = token + 1;  // пропускаем '$'
  char type = p[0];
  p++;

  // ===== ПОРТЫ: $p13=200 или $p13=200/50 =====
  if (type == 'p') {
    Serial.println("   📌 TYPE: PORT (p)");
    
    int gpio = 0;
    while (isdigit(*p)) {
      gpio = gpio * 10 + (*p - '0');
      p++;
    }
    if (*p != '=') {
      Serial.println("   ⚠️ No '=' found, skipping");
      return true;
    }
    p++;

    const char* valStart = p;
    int slashPos = -1;
    for (int i = 0; valStart[i]; i++) {
      if (valStart[i] == '/') { slashPos = i; break; }
    }

    uint16_t value = atoi(valStart);
    uint16_t duration = 0;

    if (slashPos != -1) {
      duration = atoi(valStart + slashPos + 1);
    }

    Serial.print("   GPIO: ");
    Serial.print(gpio);
    Serial.print(" Value: ");
    Serial.print(value);
    Serial.print(" Duration: ");
    Serial.println(duration);

    // ===== ПРОВЕРЯЕМ WriteProvider =====
    if (_instance->_writeProvider) {
      Serial.println("   📤 Calling WriteProvider...");
      bool result = _instance->_writeProvider(token, valStart);
      Serial.print("   WriteProvider result: ");
      Serial.println(result ? "TRUE" : "FALSE");
    } else {
      Serial.println("   ⚠️ WriteProvider is NULL!");
    }

    // ===== ПОЛУЧАЕМ ТЕКУЩЕЕ ЗНАЧЕНИЕ ПОРТА =====
    uint32_t currentValue = 0;
    char id[16];
    snprintf(id, 16, "$p%d", gpio);
    // ИСПРАВЛЕНО: используем _instance->
    _instance->getDataValue(id, currentValue);
    
    Serial.print("   Current value: ");
    Serial.println(currentValue);

    // ===== ВСТРОЕННАЯ ЛОГИКА =====
    if (duration > 0) {
      Serial.println("   🌊 Starting FADE...");
      _instance->startFade(s, gpio, value, duration, (uint16_t)currentValue);
    } else {
      Serial.println("   ⚡ Setting output instantly...");
      _instance->setOutput(gpio, value, false);
    }
    return true;
  }

  // ===== ПЕРЕМЕННЫЕ UINT: $u0=12 =====
  if (type == 'u') {
    Serial.println("   📌 TYPE: UINT (u)");
    int idx = p[0] - '0';
    if (idx < 0 || idx >= MAX_UINT_VARS || p[1] != '=') return true;
    p += 2;

    if (strcmp(p, "+1") == 0) {
      _instance->_uintVars[idx]++;
    } else if (strcmp(p, "-1") == 0) {
      _instance->_uintVars[idx]--;
    } else if (p[0] == '$' && p[1] == 'u') {
      int srcIdx = p[2] - '0';
      if (srcIdx >= 0 && srcIdx < MAX_UINT_VARS) {
        _instance->_uintVars[idx] = _instance->_uintVars[srcIdx];
      }
    } else {
      _instance->_uintVars[idx] = atoi(p);
    }
    return true;
  }

  // ===== ПЕРЕМЕННЫЕ FLOAT: $f0=12.5 =====
  if (type == 'f') {
    Serial.println("   📌 TYPE: FLOAT (f)");
    int idx = p[0] - '0';
    if (idx < 0 || idx >= MAX_FLOAT_VARS || p[1] != '=') return true;
    p += 2;

    if (strcmp(p, "+0.5") == 0) {
      _instance->_floatVars[idx] += 0.5f;
    } else if (strcmp(p, "-0.5") == 0) {
      _instance->_floatVars[idx] -= 0.5f;
    } else {
      _instance->_floatVars[idx] = atof(p);
    }
    return true;
  }

  return true;
}

bool ScriptRunner::handleGPIO(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔌 handleGPIO: ");
  Serial.println(token);
  if (!_instance) return false;
  if (s.inIf && s.skipElse) return true;
  _instance->executeToken(token, s);
  return true;
}

void ScriptRunner::update() {
  static uint32_t lastDebug = 0;
  if (millis() - lastDebug > 5000) {
    lastDebug = millis();
    int activeCount = 0;
    for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
      if (_active[i].active) activeCount++;
    }
    Serial.print("🔄 update() called, active: ");
    Serial.println(activeCount);
  }

  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active && _queueCount > 0) {
      uint8_t id = _queueId[_queueHead];
      const char* script = _queueScript[_queueHead];
      uint16_t len = _queueLen[_queueHead];
      _queueHead = (_queueHead + 1) % QUEUE_SIZE;
      _queueCount--;
      activateSlot(i, id, script, len);
    }
  }

  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    ScriptState& s = _active[i];
    if (!s.active) continue;

    uint32_t now = millis();

    if (!s.isInfinite) {
      if (now - s.startTime > 60000) {
        s.active = false;
        continue;
      }
    } else {
      s.startTime = now;
    }

    if (!(s.inIf && s.skipElse)) {
      if (now - s.lastExecutionTime < 100) {
        continue;
      }
      s.lastExecutionTime = now;
    }

    if (s.inPause) {
      if (now >= s.pauseUntil) {
        s.inPause = false;
      } else {
        continue;
      }
    }

    if (s.inWait) {
      if (now >= s.waitUntil) {
        s.inWait = false;
      } else {
        continue;
      }
    }

    if (s.inFade) {
      updateFade(s, now);
      if (s.inFade) {
        continue;
      }
    }

    if (s.pos >= s.scriptLen) {
      if (s.inLoop) {
        if (s.isInfinite) {
          s.pos = s.loopStartPos;
          s.inPause = false;
          continue;
        } else if (s.repeatCount > 0) {
          s.repeatCount--;
          if (s.repeatCount > 0) {
            s.pos = s.loopStartPos;
            s.inPause = false;
            continue;
          }
        }
      }
      s.active = false;
      continue;
    }

    const char* p = s.script + s.pos;

    while (*p == ' ' || *p == ',') {
      p++;
      s.pos++;
      if (s.pos >= s.scriptLen) {
        if (s.inLoop) {
          if (s.isInfinite) {
            s.pos = s.loopStartPos;
            s.inPause = false;
            continue;
          } else if (s.repeatCount > 0) {
            s.repeatCount--;
            if (s.repeatCount > 0) {
              s.pos = s.loopStartPos;
              s.inPause = false;
              continue;
            }
          }
        }
        s.active = false;
        return;
      }
    }

    if (*p == '\0' || s.pos >= s.scriptLen) {
      if (s.inLoop) {
        if (s.isInfinite) {
          s.pos = s.loopStartPos;
          s.inPause = false;
          continue;
        } else if (s.repeatCount > 0) {
          s.repeatCount--;
          if (s.repeatCount > 0) {
            s.pos = s.loopStartPos;
            s.inPause = false;
            continue;
          }
        }
      }
      s.active = false;
      continue;
    }

    const char* start = p;

    if (*p == '[') {
      p++;
      while (*p && *p != ']' && (p - s.script) < s.scriptLen) {
        p++;
      }
      if (*p == ']') {
        p++;
      }
    } else if (*p == ']') {
      p++;
    } else {
      while (*p && *p != ',' && *p != ']' && (p - s.script) < s.scriptLen) {
        p++;
      }
    }

    size_t len = p - start;

    if (len >= MAX_TOKEN_LEN) {
      s.pos = p - s.script;
      continue;
    }

    char token[MAX_TOKEN_LEN];
    strncpy(token, start, len);
    token[len] = '\0';
    s.pos = p - s.script;

    processToken(token, s, now);
  }
}

bool ScriptRunner::stopScript(uint8_t id) {
  int idx = findById(id);
  if (idx == -1) return false;
  _active[idx].active = false;
  return true;
}

void ScriptRunner::stopAll() {
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    _active[i].active = false;
  }
  _queueCount = 0;
  _queueHead = 0;
  _queueTail = 0;
}

bool ScriptRunner::isRunning(uint8_t id) const {
  return findById(id) != -1;
}

bool ScriptRunner::isBusy() const {
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (_active[i].active) return true;
  }
  return _queueCount > 0;
}

int ScriptRunner::findById(uint8_t id) const {
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (_active[i].active && _active[i].id == id) {
      return i;
    }
  }
  return -1;
}

bool ScriptRunner::addToQueue(uint8_t id, const char* script, uint16_t len) {
  if (_queueCount >= QUEUE_SIZE) return false;
  _queueId[_queueTail] = id;
  _queueScript[_queueTail] = script;
  _queueLen[_queueTail] = len;
  _queueTail = (_queueTail + 1) % QUEUE_SIZE;
  _queueCount++;
  return true;
}

void ScriptRunner::activateSlot(int idx, uint8_t id, const char* script, uint16_t len) {
  _active[idx].active = true;
  _active[idx].id = id;
  _active[idx].script = script;
  _active[idx].scriptLen = len;
  _active[idx].pos = 0;
  _active[idx].inPause = false;
  _active[idx].pauseUntil = 0;
  _active[idx].startTime = millis();
  _active[idx].inLoop = false;
  _active[idx].isInfinite = false;
  _active[idx].repeatCount = 0;
  _active[idx].loopStartPos = 0;
  _active[idx].lastExecutionTime = 0;

  _active[idx].inFade = false;
  _active[idx].fadeGpio = 0;
  _active[idx].fadeTarget = 0;
  _active[idx].fadeStartValue = 0;
  _active[idx].fadeStartTime = 0;
  _active[idx].fadeDuration = 0;
  _active[idx].fadeIsPwm = false;

  _active[idx].inIf = false;
  _active[idx].ifResult = false;
  _active[idx].skipElse = false;
  _active[idx].ifDepth = 0;

  _active[idx].inWait = false;
  _active[idx].waitUntil = 0;
}

DataSource* ScriptRunner::findDataSource(const char* id) {
  for (int i = 0; i < _dataSourcesCount; i++) {
    if (strcmp(_dataSources[i].id, id) == 0) {
      return &_dataSources[i];
    }
  }
  return nullptr;
}

bool ScriptRunner::getDataValue(const char* id, uint32_t& value) {
  Serial.print("📊 getDataValue: ");
  Serial.println(id);
  
  if (_readProvider) {
    DataType type;
    if (_readProvider(id, type, value)) {
      Serial.print("   ✅ from ReadProvider: ");
      Serial.println(value);
      return true;
    }
  }

  if (getDataSourceValue(id, value)) {
    Serial.print("   ✅ from DataSource: ");
    Serial.println(value);
    return true;
  }

  Serial.println("   ❌ NOT FOUND!");
  return false;
}

bool ScriptRunner::getDataSourceValue(const char* id, uint32_t& value) {
  DataSource* ds = findDataSource(id);
  if (!ds) return false;

  switch (ds->type) {
    case DATA_INT:
      value = *(int*)ds->ptr;
      return true;
    case DATA_UINT32:
      value = *(uint32_t*)ds->ptr;
      return true;
    case DATA_BOOL:
      value = *(bool*)ds->ptr ? 1 : 0;
      return true;
    case DATA_FLOAT:
      value = (uint32_t)*(float*)ds->ptr;
      return true;
    default:
      return false;
  }
}

bool ScriptRunner::isLoopStart(const char* token) {
  if (token[0] != '[') return false;
  size_t len = strlen(token);
  if (len < 2) return false;
  return token[len - 1] == ']';
}

bool ScriptRunner::isLoopEnd(const char* token) {
  if (strcmp(token, "]") == 0) return true;
  size_t len = strlen(token);
  if (len > 0 && token[len - 1] == ']') {
    return true;
  }
  return false;
}

uint8_t ScriptRunner::parseLoopCount(const char* token) {
  if (token[0] != '[') return 0;
  size_t len = strlen(token);
  if (len < 3 || token[len - 1] != ']') return 0;

  char numStr[4];
  strncpy(numStr, token + 1, len - 2);
  numStr[len - 2] = '\0';

  return atoi(numStr);
}

bool ScriptRunner::isInfiniteLoop(const char* token) {
  return strcmp(token, "[*]") == 0;
}

bool ScriptRunner::isIfStart(const char* token) {
  return strncmp(token, "if:", 3) == 0;
}

bool ScriptRunner::isElse(const char* token) {
  return strcmp(token, "else") == 0;
}

bool ScriptRunner::isEnd(const char* token) {
  return strcmp(token, "end") == 0;
}

bool ScriptRunner::parseCondition(const char* token, ScriptState& s) {
  Serial.print("🔍 parseCondition: ");
  Serial.println(token);

  const char* p = token + 3;

  const char* idStart = p;
  const char* opStart = nullptr;
  const char* opEnd = nullptr;

  while (*p) {
    if (p[0] == '>' && p[1] == '=') {
      opStart = p;
      opEnd = p + 2;
      break;
    } else if (p[0] == '<' && p[1] == '=') {
      opStart = p;
      opEnd = p + 2;
      break;
    } else if (p[0] == '=' && p[1] == '=') {
      opStart = p;
      opEnd = p + 2;
      break;
    } else if (p[0] == '!' && p[1] == '=') {
      opStart = p;
      opEnd = p + 2;
      break;
    } else if (p[0] == '>' || p[0] == '<') {
      opStart = p;
      opEnd = p + 1;
      break;
    }
    p++;
  }

  if (!opStart) {
    Serial.println("   ❌ No operator found!");
    s.ifResult = false;
    return false;
  }

  char id[32];
  size_t idLen = opStart - idStart;
  if (idLen >= 31) idLen = 30;
  strncpy(id, idStart, idLen);
  id[idLen] = '\0';

  char op1 = opStart[0];
  char op2 = (opEnd - opStart == 2) ? opStart[1] : '\0';

  uint32_t compareValue = atoi(opEnd);

  Serial.print("   id: ");
  Serial.println(id);
  Serial.print("   op: ");
  Serial.print(op1);
  if (op2) Serial.print(op2);
  Serial.println();
  Serial.print("   compareValue: ");
  Serial.println(compareValue);

  uint32_t currentValue;
  if (!getDataValue(id, currentValue)) {
    Serial.print("   ❌ Unknown data source: ");
    Serial.println(id);
    s.ifResult = false;
    return false;
  }

  Serial.print("   currentValue: ");
  Serial.println(currentValue);

  bool result = false;

  if (op1 == '>' && op2 == '=') {
    result = currentValue >= compareValue;
  } else if (op1 == '<' && op2 == '=') {
    result = currentValue <= compareValue;
  } else if (op1 == '=' && op2 == '=') {
    result = currentValue == compareValue;
  } else if (op1 == '!' && op2 == '=') {
    result = currentValue != compareValue;
  } else if (op1 == '>') {
    result = currentValue > compareValue;
  } else if (op1 == '<') {
    result = currentValue < compareValue;
  } else {
    result = false;
  }

  Serial.print("   result: ");
  Serial.println(result ? "TRUE" : "FALSE");

  s.ifResult = result;
  return result;
}

void ScriptRunner::startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, uint16_t startValue) {
  Serial.print("🌊 startFade: gpio=");
  Serial.print(gpio);
  Serial.print(" target=");
  Serial.print(target);
  Serial.print(" duration=");
  Serial.print(duration);
  Serial.print(" startValue=");
  Serial.println(startValue);

  if (duration == 0) {
    setOutput(gpio, target, false);
    return;
  }

  s.inFade = true;
  s.fadeGpio = gpio;
  s.fadeTarget = target;
  s.fadeStartValue = startValue;
  s.fadeStartTime = millis();
  s.fadeDuration = duration * 100UL;
}

void ScriptRunner::updateFade(ScriptState& s, uint32_t now) {
  if (!s.inFade) return;

  uint32_t elapsed = now - s.fadeStartTime;

  if (elapsed >= s.fadeDuration) {
    Serial.print("✅ FADE COMPLETE: gpio=");
    Serial.print(s.fadeGpio);
    Serial.print(" target=");
    Serial.println(s.fadeTarget);
    setOutput(s.fadeGpio, s.fadeTarget, false);
    s.inFade = false;
  } else {
    int16_t delta = s.fadeTarget - s.fadeStartValue;
    uint16_t currentValue;

    if (delta >= 0) {
      currentValue = s.fadeStartValue + (uint16_t)((uint32_t)delta * elapsed / s.fadeDuration);
    } else {
      currentValue = s.fadeStartValue - (uint16_t)((uint32_t)(-delta) * elapsed / s.fadeDuration);
    }

    setOutput(s.fadeGpio, currentValue, true);
  }
}

void ScriptRunner::executeToken(const char* token, ScriptState& s) {
  Serial.print("⚡ executeToken: ");
  Serial.println(token);

  int colonPos = -1;
  int slashPos = -1;

  for (int i = 0; token[i]; i++) {
    if (token[i] == ':') colonPos = i;
    if (token[i] == '/') slashPos = i;
  }

  if (colonPos == -1) {
    Serial.println("❌ No colon found");
    return;
  }

  char portStr[8];
  strncpy(portStr, token, colonPos);
  portStr[colonPos] = '\0';

  for (int i = 0; portStr[i]; i++) {
    if (!isdigit(portStr[i])) {
      Serial.print("❌ Invalid port: ");
      Serial.println(portStr);
      return;
    }
  }
  uint8_t gpio = atoi(portStr);

  char valStr[8];
  const char* valStart = token + colonPos + 1;

  bool hasDigit = false;
  for (int i = 0; valStart[i] && valStart[i] != '/' && valStart[i] != ','; i++) {
    if (isdigit(valStart[i])) hasDigit = true;
  }

  if (!hasDigit) {
    Serial.println("❌ No digit in value");
    return;
  }

  int i = 0;
  while (valStart[i] && valStart[i] != '/' && i < 7) {
    valStr[i] = valStart[i];
    i++;
  }
  valStr[i] = '\0';

  uint16_t value = atoi(valStr);
  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;

  uint16_t duration = 0;

  if (slashPos != -1) {
    const char* durStr = token + slashPos + 1;
    for (int i = 0; durStr[i]; i++) {
      if (!isdigit(durStr[i])) return;
    }
    duration = atoi(durStr);
  }

  Serial.print("   gpio=");
  Serial.print(gpio);
  Serial.print(" value=");
  Serial.print(value);
  Serial.print(" duration=");
  Serial.println(duration);

  if (duration > 0) {
    uint32_t currentValue = 0;
    char id[16];
    snprintf(id, 16, "$p%d", gpio);
    getDataValue(id, currentValue);
    startFade(s, gpio, value, duration, (uint16_t)currentValue);
  } else {
    setOutput(gpio, value, false);
  }
}

void ScriptRunner::setOutput(uint8_t gpio, uint16_t value, bool isFadeStep) {
  Serial.print("🔧 setOutput: GPIO ");
  Serial.print(gpio);
  Serial.print(" = ");
  Serial.print(value);
  Serial.print(" isFadeStep=");
  Serial.println(isFadeStep);

  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;

  uint8_t portType = (value > 1) ? PORT_TYPE_PWM : PORT_TYPE_DIGITAL;

  if (_portOutputCallback) {
    _portOutputCallback(gpio, value, portType);
  } else {
    Serial.println("   ⚠️ _portOutputCallback is NULL!");
  }

  if (!isFadeStep && _stateChangeProvider) {
    Serial.println("   🔔 Calling StateChangeProvider");
    uint32_t now = millis();
    if (now - _lastStateChangeTime >= 100) {
      _lastStateChangeTime = now;
      _stateChangeProvider(gpio, 0, value);
    }
  }
}

bool ScriptRunner::processToken(const char* token, ScriptState& s, uint32_t now) {
  Serial.print("🔍 processToken: ");
  Serial.println(token);

  for (int i = 0; _handlers[i].name != NULL || _handlers[i].handle == handleGPIO; i++) {
    if (_handlers[i].canHandle(token, s)) {
      Serial.print("   ✅ Handler: ");
      Serial.println(_handlers[i].name);
      return _handlers[i].handle(token, s, now);
    }
  }
  
  Serial.println("   ❌ No handler found!");
  return false;
}