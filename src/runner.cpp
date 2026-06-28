#include "./include/runner.h"

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

    _active[i].inWait = false;
    _active[i].waitUntil = 0;
  }
  _queueHead = 0;
  _queueTail = 0;
  _queueCount = 0;
  _portsCount = 0;
  _dataSourcesCount = 0;
  _lastStateChangeTime = 0;
}

void ScriptRunner::setDataProvider(DataProvider provider) {
  _dataProvider = provider;
}

void ScriptRunner::setLogProvider(LogProvider provider) {
  _logProvider = provider;
}

void ScriptRunner::setStateChangeProvider(StateChangeProvider provider) {
  _stateChangeProvider = provider;
}

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
    }
  }
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
  uint16_t len = strlen(script);
  if (len > 255) return false;

  int existingIdx = findById(id);

  if (existingIdx != -1) {
    switch (strategy) {
      case IGNORE:
        return true;
      case RESTART:
        activateSlot(existingIdx, id, script, len);
        return true;
      case RESTART_IF_SAME:
        if (strcmp(_active[existingIdx].script, script) == 0) {
          activateSlot(existingIdx, id, script, len);
          return true;
        }
        break;
      case ADD_QUEUE:
        return addToQueue(id, script, len);
    }
  }

  for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
    if (!_active[i].active) {
      activateSlot(i, id, script, len);
      return true;
    }
  }

  return addToQueue(id, script, len);
}

bool ScriptRunner::isWaitStart(const char* token) {
  return strncmp(token, "wait:", 5) == 0;
}

bool ScriptRunner::handleWait(const char* token, ScriptState& s, uint32_t now) {
  const char* valStr = token + 5;
  uint32_t duration = 0;

  uint32_t num = 0;
  while (isdigit(*valStr)) {
    num = num * 10 + (*valStr - '0');
    valStr++;
  }

  if (*valStr == 's' || *valStr == 'S') {
    duration = num * 1000;
  } else if (*valStr == 'm' || *valStr == 'M') {
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

void ScriptRunner::update() {
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

    if (token[0] == 'p' || token[0] == 'P') {
      if (s.inIf && s.skipElse) {
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
        }
      }
      continue;
    }

    if (isWaitStart(token)) {
      if (s.inIf && s.skipElse) {
        continue;
      }
      handleWait(token, s, now);
      continue;
    }

    if (strncmp(token, "log:", 4) == 0) {
      if (s.inIf && s.skipElse) {
        continue;
      }
      if (_logProvider) {
        const char* msg = token + 4;

        if (strstr(msg, "?:") != nullptr) {
          char buffer[256];
          const char* p = msg;
          int pos = 0;

          while (*p && pos < 250) {
            if (p[0] == '?' && p[1] == ':') {
              p += 2;
              while (*p == ' ') p++;

              const char* idStart = p;
              while (*p && *p != ',' && *p != ' ' && *p != '\0' && *p != '?' && *p != ':') {
                p++;
              }

              if (p[0] == '?' && p[1] == ':') {
                p = idStart;
                buffer[pos++] = '?';
                buffer[pos++] = ':';
                continue;
              }

              char id[16];
              int idLen = p - idStart;
              if (idLen >= 16) idLen = 15;
              strncpy(id, idStart, idLen);
              id[idLen] = '\0';
              id[idLen] = '\0';

              if (idLen > 0) {
                uint32_t value;
                DataType type;
                if (_dataProvider && _dataProvider(id, type, value)) {
                  pos += snprintf(buffer + pos, 250 - pos, "%u", value);
                } else {
                  pos += snprintf(buffer + pos, 250 - pos, "???");
                }
              } else {
                buffer[pos++] = '?';
                buffer[pos++] = ':';
              }
            } else {
              buffer[pos++] = *p;
              p++;
            }
          }
          buffer[pos] = '\0';

          _logProvider(buffer);
        } else {
          _logProvider(msg);
        }
      }
      continue;
    }

    if (isLoopStart(token)) {
      s.loopStartPos = s.pos;
      s.inLoop = true;

      if (isInfiniteLoop(token)) {
        s.isInfinite = true;
        s.repeatCount = 0;
        s.startTime = now;
      } else {
        s.isInfinite = false;
        s.repeatCount = parseLoopCount(token);
        if (s.repeatCount == 0) {
          s.inLoop = false;
        }
      }
      continue;
    }

    if (isLoopEnd(token)) {
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
        s.inLoop = false;
        s.isInfinite = false;
        s.repeatCount = 0;
      }
      continue;
    }

    if (isIfStart(token)) {
      s.ifDepth++;
      s.inIf = true;

      if (parseCondition(token, s)) {
        s.skipElse = false;
      } else {
        s.skipElse = true;
      }
      continue;
    }

    if (isElse(token)) {
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

    if (isEnd(token)) {
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

    if (s.inIf && s.skipElse) {
      continue;
    }

    executeToken(token, s);
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

  return true;
}

uint16_t ScriptRunner::getPortValue(uint8_t gpio) const {
  const PortState* p = findPort(gpio);
  if (!p) return 0;
  return p->value;
}

void ScriptRunner::syncInputs() {
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
  _active[idx].fadeIsToggle = false;
  _active[idx].fadeToggleValue = 0;

  _active[idx].inIf = false;
  _active[idx].ifResult = false;
  _active[idx].skipElse = false;
  _active[idx].ifDepth = 0;

  _active[idx].inWait = false;
  _active[idx].waitUntil = 0;
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

bool ScriptRunner::getDataValue(const char* id, uint32_t& value) {
  if (getPortValue(id, value)) {
    return true;
  }

  if (getDataSourceValue(id, value)) {
    return true;
  }

  if (_dataProvider) {
    DataType type;
    if (_dataProvider(id, type, value)) {
      return true;
    }
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
  const char* p = token + 3;

  if (p[0] != '?' || p[1] != ':') {
    s.ifResult = false;
    return false;
  }
  p += 2;

  const char* idStart = p;
  while (*p && *p != '>' && *p != '<' && *p != '=' && *p != '!') {
    p++;
  }

  if (*p == '\0') {
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

  uint32_t currentValue;
  if (!getDataValue(id, currentValue)) {
    s.ifResult = false;
    return false;
  }

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
      result = false;
      break;
  }

  s.ifResult = result;
  return result;
}

void ScriptRunner::startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, bool isToggle, uint16_t toggleValue) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return;

  if (p->mode != GPIO_MODE_PWM) {
    setOutput(gpio, target, false);
    return;
  }

  if (duration == 0) {
    setOutput(gpio, target, false);
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
  int starPos = -1;
  int colonPos = -1;
  int slashPos = -1;

  for (int i = 0; token[i]; i++) {
    if (token[i] == ':') colonPos = i;
    if (token[i] == '*') starPos = i;
    if (token[i] == '/') slashPos = i;
  }

  if (colonPos == -1) return;

  char portStr[8];
  strncpy(portStr, token, colonPos);
  portStr[colonPos] = '\0';

  for (int i = 0; portStr[i]; i++) {
    if (!isdigit(portStr[i])) return;
  }
  uint8_t gpio = atoi(portStr);

  char valStr[8];
  const char* valStart = token + colonPos + 1;

  if (*valStart == '*') {
    valStart++;
  }

  bool hasDigit = false;
  for (int i = 0; valStart[i] && valStart[i] != '/' && valStart[i] != ','; i++) {
    if (isdigit(valStart[i])) hasDigit = true;
  }

  if (!hasDigit) return;

  int i = 0;
  while (valStart[i] && valStart[i] != '/' && i < 7) {
    valStr[i] = valStart[i];
    i++;
  }
  valStr[i] = '\0';

  uint16_t value = atoi(valStr);
  if (value > MAX_PWM_VALUE) return;

  uint16_t duration = 0;
  if (slashPos != -1) {
    const char* durStr = token + slashPos + 1;
    for (int i = 0; durStr[i]; i++) {
      if (!isdigit(durStr[i])) return;
    }
    duration = atoi(durStr);
  }

  if (strcmp(portStr, "a") == 0) {
    for (int i = 0; i < _portsCount; i++) {
      if (_ports[i].isOutput) {
        setOutput(_ports[i].gpio, 0, false);
      }
    }
    return;
  }

  PortState* p = findPort(gpio);
  if (!p) return;

  if (!p->isOutput) return;

  bool isToggle = (starPos != -1);
  bool isFade = (duration > 0);

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
      setOutput(gpio, value, false);
    }
  }
}

void ScriptRunner::handleToggle(uint8_t gpio, uint16_t toggleValue) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return;

  if (p->mode == GPIO_MODE_PWM) {
    if (p->value == 0) {
      setOutput(gpio, toggleValue, false);
    } else {
      setOutput(gpio, 0, false);
    }
  } else {
    if (p->value == 0) {
      setOutput(gpio, 255, false);
    } else {
      setOutput(gpio, 0, false);
    }
  }
}

void ScriptRunner::setOutput(uint8_t gpio, uint16_t value, bool isFadeStep) {
  PortState* p = findPort(gpio);
  if (!p || !p->isOutput) return;

  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;

  uint16_t oldValue = p->value;
  p->value = value;

  if (p->mode == GPIO_MODE_PWM) {
    analogWrite(gpio, value);
  } else {
    digitalWrite(gpio, value > 0 ? HIGH : LOW);
  }

  if (!isFadeStep && _stateChangeProvider && oldValue != value) {
    uint32_t now = millis();
    if (now - _lastStateChangeTime >= 100) {
      _lastStateChangeTime = now;
      _stateChangeProvider(gpio, oldValue, value);
    }
  }
}