// ==================== runner.cpp ====================
#include "./include/runner.h"

// ============================================================
// ===== РЕАЛИЗАЦИЯ МЕТОДОВ =====
// ============================================================

// ---- Конструктор ----
ScriptRunner::ScriptRunner(ScriptConflict defaultStrategy)
    : _defaultStrategy(defaultStrategy) {
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
    _active[i].fadeIsToggle = false;
    _active[i].fadeToggleValue = 0;

    _active[i].inIf = false;
    _active[i].ifResult = false;
    _active[i].skipElse = false;
    _active[i].ifDepth = 0;
  }
  _queueHead = 0;
  _queueTail = 0;
  _queueCount = 0;
  _portsCount = 0;
  _dataSourcesCount = 0;
}

// ---- Инициализация портов ----
void ScriptRunner::initPorts(Port* ports, uint8_t count) {
  _portsCount = 0;
  for (int i = 0; i < count && i < MAX_PORTS; i++) {
    if (ports[i].mode == GPIO_MODE_OUTPUT ||
        ports[i].mode == GPIO_MODE_OUTPUT_OPEN_DRAIN ||
        ports[i].mode == GPIO_MODE_PWM) {
      _ports[_portsCount].gpio = ports[i].gpio;
      _ports[_portsCount].mode = ports[i].mode;
      _ports[_portsCount].value = ports[i].value;
      _ports[_portsCount].isOutput = true;
      _portsCount++;

#if RUNNER_DEBUG
      Serial.print("✅ Port initialized: GPIO ");
      Serial.print(ports[i].gpio);
      Serial.print(" mode ");
      Serial.println(ports[i].mode);
#endif
    }
  }
#if RUNNER_DEBUG
  Serial.print("✅ Total ports initialized: ");
  Serial.println(_portsCount);
#endif
}

// ---- Добавление источника данных ----
void ScriptRunner::addDataSource(const char* id, DataType type, void* ptr) {
  if (_dataSourcesCount >= MAX_DATA_SOURCES) {
#if RUNNER_DEBUG
    Serial.println("⚠️ Too many data sources!");
#endif
    return;
  }

  strncpy(_dataSources[_dataSourcesCount].id, id, 15);
  _dataSources[_dataSourcesCount].id[15] = '\0';
  _dataSources[_dataSourcesCount].type = type;
  _dataSources[_dataSourcesCount].ptr = ptr;
  _dataSourcesCount++;

#if RUNNER_DEBUG
  Serial.print("📊 Data source registered: ");
  Serial.println(id);
#endif
}

// ---- Добавление сценария ----
bool ScriptRunner::addScript(uint8_t id, const char* script, ScriptConflict strategy) {
#if RUNNER_DEBUG
  Serial.print("📝 addScript ID:");
  Serial.print(id);
  Serial.print(" script: ");
  Serial.println(script);
#endif

  uint16_t len = strlen(script);
  if (len > 255) {
#if RUNNER_DEBUG
    Serial.println("❌ Script too long!");
#endif
    return false;
  }

  int existingIdx = findById(id);

  if (existingIdx != -1) {
#if RUNNER_DEBUG
    Serial.print("   Script ID ");
    Serial.print(id);
    Serial.println(" already exists");
#endif

    switch (strategy) {
      case IGNORE:
#if RUNNER_DEBUG
        Serial.println("   IGNORE");
#endif
        return true;
      case RESTART:
#if RUNNER_DEBUG
        Serial.println("   RESTART");
#endif
        activateSlot(existingIdx, id, script, len);
        return true;
      case RESTART_IF_SAME:
        if (strcmp(_active[existingIdx].script, script) == 0) {
#if RUNNER_DEBUG
          Serial.println("   RESTART_IF_SAME: same script, restarting");
#endif
          activateSlot(existingIdx, id, script, len);
          return true;
        }
        break;
      case ADD_QUEUE:
#if RUNNER_DEBUG
        Serial.println("   ADD_QUEUE");
#endif
        return addToQueue(id, script, len);
    }
  }

  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active) {
#if RUNNER_DEBUG
      Serial.print("   Activating slot ");
      Serial.println(i);
#endif
      activateSlot(i, id, script, len);
      return true;
    }
  }

#if RUNNER_DEBUG
  Serial.println("   No free slots, adding to queue");
#endif
  return addToQueue(id, script, len);
}

// ---- Обновление (выполнение) ----
void ScriptRunner::update() {
  // 1. Заполняем пустые слоты из очереди
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

  // 2. Round-robin по активным сценариям
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    ScriptState& s = _active[i];
    if (!s.active) continue;

    uint32_t now = millis();

    // ===== ЗАЩИТА 1: Глобальный таймаут =====
    if (!s.isInfinite) {
      if (now - s.startTime > 60000) {
#if RUNNER_DEBUG
        Serial.print("⏰ Script ");
        Serial.print(s.id);
        Serial.println(" timeout (60s)");
#endif
        s.active = false;
        continue;
      }
    } else {
      s.startTime = now;
    }

    // ===== ЗАЩИТА 2: Минимальный интервал 100 мс =====
    // Пропускаем защиту, если мы в skipElse (чтобы не создавать задержек)
    if (!(s.inIf && s.skipElse)) {
      if (now - s.lastExecutionTime < 100) {
        continue;
      }
      s.lastExecutionTime = now;
    }

    // Проверка паузы
    if (s.inPause) {
      if (now >= s.pauseUntil) {
        s.inPause = false;
      } else {
        continue;
      }
    }

    // ===== ПРОВЕРКА FADE =====
    if (s.inFade) {
      updateFade(s, now);
      if (s.inFade) {
        continue;
      }
    }

    // Проверка окончания сценария
    if (s.pos >= s.scriptLen) {
#if RUNNER_DEBUG
      Serial.print("🔚 Script ");
      Serial.print(s.id);
      Serial.print(" pos >= len");
#endif

      if (s.inLoop) {
        if (s.isInfinite) {
#if RUNNER_DEBUG
          Serial.println(" ♾️ Infinite loop restart");
#endif
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
#if RUNNER_DEBUG
      Serial.println(" ✅ Script completed");
#endif
      s.active = false;
      continue;
    }

    // ---- Парсинг токена ----
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

    // ---- Парсинг токена с поддержкой циклов ----
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

// ===== ДЕБАГ =====
#if RUNNER_DEBUG
    Serial.print("🔍 [");
    Serial.print(s.id);
    Serial.print("] Token: '");
    Serial.print(token);
    Serial.print("' | inIf: ");
    Serial.print(s.inIf);
    Serial.print(" | ifResult: ");
    Serial.print(s.ifResult);
    Serial.print(" | skipElse: ");
    Serial.print(s.skipElse);
    Serial.print(" | ifDepth: ");
    Serial.println(s.ifDepth);
#endif

    // ---- Обработка токенов ----

    // ---- Пауза ----
    if (token[0] == 'p' || token[0] == 'P') {
      if (s.inIf && s.skipElse) {
#if RUNNER_DEBUG
        Serial.println("   ⏭️ SKIPPED (skipElse)");
#endif
        continue;
      }

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
#if RUNNER_DEBUG
          Serial.print("   ⏸️ Pause ");
          Serial.print(val * 100);
          Serial.println("ms");
#endif
        }
      }
      continue;
    }

    // ---- Цикл: [N] или [*] ----
    if (isLoopStart(token)) {
#if RUNNER_DEBUG
      Serial.print("   🔁 LOOP START: ");
      Serial.println(token);
#endif

      s.loopStartPos = s.pos;
      s.inLoop = true;

      if (isInfiniteLoop(token)) {
        s.isInfinite = true;
        s.repeatCount = 0;
        s.startTime = now;
#if RUNNER_DEBUG
        Serial.println("   ♾️ INFINITE LOOP");
#endif
      } else {
        s.isInfinite = false;
        s.repeatCount = parseLoopCount(token);
#if RUNNER_DEBUG
        Serial.print("   🔢 REPEAT: ");
        Serial.println(s.repeatCount);
#endif
        if (s.repeatCount == 0) {
          s.inLoop = false;
#if RUNNER_DEBUG
          Serial.println("   ⚠️ repeatCount = 0, loop disabled");
#endif
        }
      }
      continue;
    }

    // ---- Конец цикла ----
    if (isLoopEnd(token)) {
#if RUNNER_DEBUG
      Serial.print("   🔚 LOOP END: ");
      Serial.print(token);
      Serial.print(" inLoop=");
      Serial.print(s.inLoop);
      Serial.print(" repeatCount=");
      Serial.print(s.repeatCount);
      Serial.print(" isInfinite=");
      Serial.println(s.isInfinite);
#endif

      if (s.inLoop) {
        if (s.isInfinite) {
#if RUNNER_DEBUG
          Serial.println("   ♾️ Infinite loop - restart");
#endif
          s.pos = s.loopStartPos;
          s.inPause = false;
          continue;
        } else if (s.repeatCount > 0) {
          s.repeatCount--;
#if RUNNER_DEBUG
          Serial.print("   🔢 repeatCount decreased to ");
          Serial.println(s.repeatCount);
#endif
          if (s.repeatCount > 0) {
            s.pos = s.loopStartPos;
            s.inPause = false;
            continue;
          }
        }
#if RUNNER_DEBUG
        Serial.println("   ✅ Loop complete");
#endif
        s.inLoop = false;
        s.isInfinite = false;
        s.repeatCount = 0;
      }
      continue;
    }

    // ---- IF ----
    if (isIfStart(token)) {
#if RUNNER_DEBUG
      Serial.print("   🔀 IF: ");
      Serial.println(token);
#endif

      s.ifDepth++;
      s.inIf = true;

      if (parseCondition(token, s)) {
        s.skipElse = false;
      } else {
        s.skipElse = true;
      }
      continue;
    }

    // ---- ELSE ----
    if (isElse(token)) {
#if RUNNER_DEBUG
      Serial.print("   🔀 ELSE: ");
      Serial.println(token);
#endif

      if (s.inIf && s.ifDepth > 0) {
        if (s.ifResult) {
          s.skipElse = true;
        } else {
          s.skipElse = false;
          s.ifResult = true;
        }
      }
      continue;
    }

    // ---- END ----
    if (isEnd(token)) {
#if RUNNER_DEBUG
      Serial.print("   🔀 END: ");
      Serial.println(token);
#endif

      if (s.ifDepth > 0) {
        s.ifDepth--;
        if (s.ifDepth == 0) {
          s.inIf = false;
          s.skipElse = false;
          s.ifResult = false;
        }
      }
      continue;
    }

    // ---- Пропуск токенов при skipElse ----
    if (s.inIf && s.skipElse) {
#if RUNNER_DEBUG
      Serial.println("   ⏭️ SKIPPED (skipElse)");
#endif
      continue;
    }

// ---- Обычное действие ----
#if RUNNER_DEBUG
    Serial.print("   ⚡ EXECUTE: ");
    Serial.println(token);
#endif
    executeToken(token, s);
  }
}

// ---- Остановка сценария по ID ----
bool ScriptRunner::stopScript(uint8_t id) {
  int idx = findById(id);
  if (idx == -1) return false;
  _active[idx].active = false;
#if RUNNER_DEBUG
  Serial.print("🛑 Script ");
  Serial.print(id);
  Serial.println(" stopped");
#endif
  return true;
}

// ---- Остановка всех сценариев ----
void ScriptRunner::stopAll() {
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    _active[i].active = false;
  }
  _queueCount = 0;
  _queueHead = 0;
  _queueTail = 0;
#if RUNNER_DEBUG
  Serial.println("🛑 All scripts stopped");
#endif
}

// ---- Проверка выполнения сценария ----
bool ScriptRunner::isRunning(uint8_t id) const {
  return findById(id) != -1;
}

// ---- Проверка занятости ----
bool ScriptRunner::isBusy() const {
  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (_active[i].active) return true;
  }
  return _queueCount > 0;
}

// ============================================================
// ===== ПУБЛИЧНЫЕ МЕТОДЫ =====
// ============================================================

bool ScriptRunner::setPortValue(uint8_t gpio, uint16_t value) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return false;

  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;

  p->value = value;

  if (p->mode == GPIO_MODE_PWM) {
    analogWrite(gpio, value);
  } else {
    digitalWrite(gpio, value > 0 ? HIGH : LOW);
  }

#if RUNNER_DEBUG
  Serial.print("🔧 setPortValue GPIO ");
  Serial.print(gpio);
  Serial.print(" = ");
  Serial.println(value);
#endif

  return true;
}

uint16_t ScriptRunner::getPortValue(uint8_t gpio) const {
  const PortState* p = findPort(gpio);
  if (!p) return 0;
  return p->value;
}

void ScriptRunner::syncInputs() {
  // Для входов - можно читать состояния при необходимости
}

// ============================================================
// ===== ПРИВАТНЫЕ МЕТОДЫ =====
// ============================================================

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
  _active[idx].fadeIsToggle = false;
  _active[idx].fadeToggleValue = 0;

  _active[idx].inIf = false;
  _active[idx].ifResult = false;
  _active[idx].skipElse = false;
  _active[idx].ifDepth = 0;

#if RUNNER_DEBUG
  Serial.print("▶️ Script ");
  Serial.print(id);
  Serial.print(" activated, script: ");
  Serial.println(script);
#endif
}

PortState* ScriptRunner::findPort(uint8_t gpio) {
  for (int i = 0; i < _portsCount; i++) {
    if (_ports[i].gpio == gpio) {
      return &_ports[i];
    }
  }
  return nullptr;
}

const PortState* ScriptRunner::findPort(uint8_t gpio) const {
  for (int i = 0; i < _portsCount; i++) {
    if (_ports[i].gpio == gpio) {
      return &_ports[i];
    }
  }
  return nullptr;
}

DataSource* ScriptRunner::findDataSource(const char* id) {
  for (int i = 0; i < _dataSourcesCount; i++) {
    if (strcmp(_dataSources[i].id, id) == 0) {
      return &_dataSources[i];
    }
  }
  return nullptr;
}

// ============================================================
// ===== МЕТОДЫ ДЛЯ ДАННЫХ =====
// ============================================================

bool ScriptRunner::getDataValue(const char* id, uint32_t& value) {
  if (getPortValue(id, value)) {
    return true;
  }

  if (getDataSourceValue(id, value)) {
    return true;
  }

  return false;
}

bool ScriptRunner::getPortValue(const char* id, uint32_t& value) {
  bool isNumber = true;
  for (int i = 0; id[i]; i++) {
    if (!isdigit(id[i])) {
      isNumber = false;
      break;
    }
  }

  if (isNumber) {
    uint8_t gpio = atoi(id);
    PortState* p = findPort(gpio);
    if (!p) return false;

    if (p->mode == GPIO_MODE_INPUT || p->mode == GPIO_MODE_INPUT_PULLUP) {
      value = digitalRead(gpio);
      return true;
    } else {
      value = p->value;
      return true;
    }
  }

  if ((id[0] == 'A' || id[0] == 'a') && isdigit(id[1])) {
    uint8_t pin = atoi(id + 1);
    value = analogRead(pin);
    return true;
  }

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

// ============================================================
// ===== МЕТОДЫ ДЛЯ ЦИКЛОВ =====
// ============================================================

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

// ============================================================
// ===== МЕТОДЫ ДЛЯ УСЛОВИЙ =====
// ============================================================

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
  const char* p = token + 3;

  if (p[0] != '?' || p[1] != ':') {
#if RUNNER_DEBUG
    Serial.println("   ⚠️ Invalid condition format (expected '?:')");
#endif
    s.ifResult = false;
    return false;
  }
  p += 2;

  const char* idStart = p;
  while (*p && *p != '>' && *p != '<' && *p != '=' && *p != '!') {
    p++;
  }

  if (*p == '\0') {
#if RUNNER_DEBUG
    Serial.println("   ⚠️ Invalid condition format (no operator)");
#endif
    s.ifResult = false;
    return false;
  }

  char id[16];
  size_t idLen = p - idStart;
  if (idLen >= 16) idLen = 15;
  strncpy(id, idStart, idLen);
  id[idLen] = '\0';

  char op = *p;
  p++;

  uint32_t compareValue = atoi(p);

#if RUNNER_DEBUG
  Serial.print("   📊 Condition: ");
  Serial.print(id);
  Serial.print(" ");
  Serial.print(op);
  Serial.print(" ");
  Serial.println(compareValue);
#endif

  uint32_t currentValue;
  if (!getDataValue(id, currentValue)) {
#if RUNNER_DEBUG
    Serial.print("   ⚠️ Unknown data source: ");
    Serial.println(id);
#endif
    s.ifResult = false;
    return false;
  }

#if RUNNER_DEBUG
  Serial.print("   📊 Current value: ");
  Serial.println(currentValue);
#endif

  bool result = false;
  switch (op) {
    case '>':
      result = currentValue > compareValue;
      break;
    case '<':
      result = currentValue < compareValue;
      break;
    case '=':
      result = currentValue == compareValue;
      break;
    case '!':
      result = currentValue != compareValue;
      break;
    default:
#if RUNNER_DEBUG
      Serial.print("   ⚠️ Unknown operator: ");
      Serial.println(op);
#endif
      result = false;
      break;
  }

  s.ifResult = result;

#if RUNNER_DEBUG
  Serial.print("   📊 Result: ");
  Serial.println(result ? "TRUE" : "FALSE");
#endif

  return result;
}

// ============================================================
// ===== МЕТОДЫ ДЛЯ FADE =====
// ============================================================

void ScriptRunner::startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, bool isToggle, uint16_t toggleValue) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return;

  if (p->mode != GPIO_MODE_PWM) {
    setOutput(gpio, target);
    return;
  }

  if (duration == 0) {
    setOutput(gpio, target);
    return;
  }

  s.inFade = true;
  s.fadeGpio = gpio;
  s.fadeTarget = target;
  s.fadeStartValue = p->value;
  s.fadeStartTime = millis();
  s.fadeDuration = duration * 100UL;
  s.fadeIsToggle = isToggle;
  s.fadeToggleValue = toggleValue;

#if RUNNER_DEBUG
  Serial.print("   🌊 FADE START: GPIO ");
  Serial.print(gpio);
  Serial.print(" from ");
  Serial.print(s.fadeStartValue);
  Serial.print(" to ");
  Serial.print(target);
  Serial.print(" over ");
  Serial.print(s.fadeDuration);
  Serial.println("ms");
#endif
}

void ScriptRunner::updateFade(ScriptState& s, uint32_t now) {
  if (!s.inFade) return;

  PortState* p = findPort(s.fadeGpio);
  if (!p || !p->isOutput) {
    s.inFade = false;
    return;
  }

  uint32_t elapsed = now - s.fadeStartTime;

  if (elapsed >= s.fadeDuration) {
    setOutput(s.fadeGpio, s.fadeTarget);
    s.inFade = false;

#if RUNNER_DEBUG
    Serial.print("   🌊 FADE COMPLETE: GPIO ");
    Serial.print(s.fadeGpio);
    Serial.print(" = ");
    Serial.println(s.fadeTarget);
#endif
  } else {
    int16_t delta = s.fadeTarget - s.fadeStartValue;
    uint16_t currentValue;

    if (delta >= 0) {
      currentValue = s.fadeStartValue + (uint16_t)((uint32_t)delta * elapsed / s.fadeDuration);
    } else {
      currentValue = s.fadeStartValue - (uint16_t)((uint32_t)(-delta) * elapsed / s.fadeDuration);
    }

    setOutput(s.fadeGpio, currentValue);
  }
}

// ============================================================
// ===== ВЫПОЛНЕНИЕ ТОКЕНА =====
// ============================================================

void ScriptRunner::executeToken(const char* token, ScriptState& s) {
  int starPos = -1;
  int colonPos = -1;
  int slashPos = -1;

  for (int i = 0; token[i]; i++) {
    if (token[i] == ':') colonPos = i;
    if (token[i] == '*') starPos = i;
    if (token[i] == '/') slashPos = i;
  }

  if (colonPos == -1) {
#if RUNNER_DEBUG
    Serial.print("⚠️ Invalid token (no colon): ");
    Serial.println(token);
#endif
    return;
  }

  char portStr[8];
  strncpy(portStr, token, colonPos);
  portStr[colonPos] = '\0';

  for (int i = 0; portStr[i]; i++) {
    if (!isdigit(portStr[i])) {
#if RUNNER_DEBUG
      Serial.print("⚠️ Invalid port: ");
      Serial.println(portStr);
#endif
      return;
    }
  }
  uint8_t gpio = atoi(portStr);

  char valStr[8];
  if (slashPos != -1) {
    strncpy(valStr, token + colonPos + 1, slashPos - colonPos - 1);
    valStr[slashPos - colonPos - 1] = '\0';
  } else {
    strcpy(valStr, token + colonPos + 1);
  }

  for (int i = 0; valStr[i]; i++) {
    if (!isdigit(valStr[i]) && valStr[i] != '*') {
#if RUNNER_DEBUG
      Serial.print("⚠️ Invalid value: ");
      Serial.println(valStr);
#endif
      return;
    }
  }
  uint16_t value = atoi(valStr);
  if (value > MAX_PWM_VALUE) {
#if RUNNER_DEBUG
    Serial.print("⚠️ Value too high: ");
    Serial.println(value);
#endif
    return;
  }

  uint16_t duration = 0;
  if (slashPos != -1) {
    const char* durStr = token + slashPos + 1;
    for (int i = 0; durStr[i]; i++) {
      if (!isdigit(durStr[i])) {
#if RUNNER_DEBUG
        Serial.print("⚠️ Invalid duration: ");
        Serial.println(durStr);
#endif
        return;
      }
    }
    duration = atoi(durStr);
  }

  if (strcmp(portStr, "a") == 0) {
#if RUNNER_DEBUG
    Serial.println("   🔘 ALL OFF");
#endif
    for (int i = 0; i < _portsCount; i++) {
      if (_ports[i].isOutput) {
        setOutput(_ports[i].gpio, 0);
      }
    }
    return;
  }

  PortState* p = findPort(gpio);
  if (!p) {
#if RUNNER_DEBUG
    Serial.print("⚠️ Port GPIO ");
    Serial.print(gpio);
    Serial.println(" not found!");
#endif
    return;
  }

  if (!p->isOutput) {
#if RUNNER_DEBUG
    Serial.print("⚠️ Port GPIO ");
    Serial.print(gpio);
    Serial.println(" is not output!");
#endif
    return;
  }

  bool isToggle = (starPos != -1);
  bool isFade = (duration > 0);

#if RUNNER_DEBUG
  Serial.print("   🔘 GPIO ");
  Serial.print(gpio);
  Serial.print(" value ");
  Serial.print(value);
  if (isToggle) Serial.print(" (toggle)");
  if (isFade) {
    Serial.print(" (fade ");
    Serial.print(duration * 100);
    Serial.print("ms)");
  }
  Serial.println();
#endif

  if (isFade) {
    uint16_t target;
    if (isToggle) {
      target = (p->value == 0) ? value : 0;
      startFade(s, gpio, target, duration, true, value);
    } else {
      startFade(s, gpio, value, duration, false, 0);
    }
  } else {
    if (isToggle) {
      handleToggle(gpio, value);
    } else {
      setOutput(gpio, value);
    }
  }
}

void ScriptRunner::handleToggle(uint8_t gpio, uint16_t toggleValue) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return;

#if RUNNER_DEBUG
  Serial.print("   🔄 Toggle GPIO ");
  Serial.print(gpio);
  Serial.print(" current value: ");
  Serial.println(p->value);
#endif

  if (p->mode == GPIO_MODE_PWM) {
    if (p->value == 0) {
      setOutput(gpio, toggleValue);
    } else {
      setOutput(gpio, 0);
    }
  } else {
    if (p->value == 0) {
      setOutput(gpio, 255);
    } else {
      setOutput(gpio, 0);
    }
  }
}

void ScriptRunner::setOutput(uint8_t gpio, uint16_t value) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return;

  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;

  p->value = value;

#if RUNNER_DEBUG
  Serial.print("   ⚡ setOutput GPIO ");
  Serial.print(gpio);
  Serial.print(" = ");
  Serial.println(value);
#endif

  if (p->mode == GPIO_MODE_PWM) {
    analogWrite(gpio, value);
  } else {
    digitalWrite(gpio, value > 0 ? HIGH : LOW);
  }
}