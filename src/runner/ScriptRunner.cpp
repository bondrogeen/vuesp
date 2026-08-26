#ifdef ARDUINO
#include "./ScriptRunner.h"
#else
#include "ScriptRunner.h"
#endif

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

ScriptRunner* ScriptRunner::_instance = nullptr;

uint32_t ScriptRunner::hash(const char* str) {
  if (!str) return 0;
  uint32_t hash = 5381;
  while (*str) hash = ((hash << 5) + hash) + (uint8_t)*str++;
  return hash;
}

static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool isAlphaNum(char c) { return isAlpha(c) || isDigit(c) || c == '_'; }

uint32_t ScriptRunner::parseUint(const char** p) const {
  uint32_t val = 0;
  while (isDigit(**p)) {
    val = val * 10 + (**p - '0');
    (*p)++;
  }
  return val;
}

int32_t ScriptRunner::parseInt(const char** p) const {
  int32_t val = 0;
  bool negative = false;
  if (**p == '-') {
    negative = true;
    (*p)++;
  }
  while (isDigit(**p)) {
    val = val * 10 + (**p - '0');
    (*p)++;
  }
  return negative ? -val : val;
}

float ScriptRunner::parseFloat(const char** p) const {
  char* end;
  float val = strtof(*p, &end);
  if (end > *p) {
    *p = end;
    return val;
  }
  return 0.0f;
}

bool ScriptRunner::parseString(const char** p, char* buf) const {
  if (**p != '\'') return false;
  (*p)++;
  const char* start = *p;
  while (**p && **p != '\'') (*p)++;
  uint16_t len = *p - start;
  if (len >= MAX_STRING_LEN) len = MAX_STRING_LEN - 1;
  strncpy(buf, start, len);
  buf[len] = '\0';
  if (**p == '\'') (*p)++;
  return true;
}

uint32_t ScriptRunner::parseTime(const char* str) const {
  const char* p = str;
  uint32_t value = 0;
  while (isDigit(*p)) {
    value = value * 10 + (*p - '0');
    p++;
  }
  char unit = *p;
  switch (unit) {
    case 'u':
      return value;
    case 's':
      return value * 1000;
    case 'm':
      return value * 60000;
    case 'h':
      return value * 3600000;
    default:
      return value * 1000;
  }
}

Params ScriptRunner::parseParams(const char* str) const {
  Params result;
  result.count = 0;
  const char* open = strchr(str, '(');
  const char* close = strchr(str, ')');
  if (!open || !close) return result;
  const char* p = open + 1;
  while (p < close && result.count < MAX_PARAMS) {
    while (*p == ' ') p++;
    if (p >= close) break;
    const char* start = p;
    if (*p == '\'') {
      p++;
      while (p < close && *p != '\'') p++;
      if (p < close && *p == '\'') p++;
    } else {
      while (p < close && *p != ',') p++;
    }
    uint16_t len = p - start;
    if (len > 31) len = 31;
    strncpy(result.values[result.count], start, len);
    result.values[result.count][len] = '\0';
    result.count++;
    if (p < close && *p == ',') p++;
  }
  return result;
}

bool ScriptRunner::parseVarUint(uint8_t idx, int32_t& result) {
  if (idx >= MAX_UINT_VARS) return false;
  result = (int32_t)_ctx.uintVars[idx];
  return true;
}

bool ScriptRunner::parseVarInt(uint8_t idx, int32_t& result) {
  if (idx >= MAX_INT_VARS) return false;
  result = _ctx.intVars[idx];
  return true;
}

bool ScriptRunner::parseVarFloat(uint8_t idx, int32_t& result) {
  if (idx >= MAX_FLOAT_VARS) return false;
  result = (int32_t)((float)_ctx.floatVars[idx] * 1000.0f);
  return true;
}

bool ScriptRunner::parseVarString(uint8_t idx, int32_t& result) {
  if (idx >= MAX_STRING_VARS) return false;
  if (_ctx.stringVars[idx][0] == '\0') {
    result = 0;
    return true;
  }
  result = (int32_t)hash(_ctx.stringVars[idx]);
  return true;
}

bool ScriptRunner::parseVarPort(uint8_t idx, int32_t& result, uint8_t slot) {
  uint16_t val = 0;
  if (!_portProvider || !_portProvider(idx, PORT_READ, val)) return false;
#if ENABLE_PORT_LOGGING && ENABLE_LOGGING
  logPortAction(idx, PORT_READ, val, slot);
#endif
  result = (int32_t)val;
  return true;
}

bool ScriptRunner::parseVarData(const char* start, int32_t& result, const char** p, DataKind expectedKind) {
  if (!_dataProvider) return false;
  const char* end = start;
  while (isAlphaNum(*end) || *end == '$') end++;
  uint16_t len = end - start;
  if (len > 31) len = 31;
  strncpy(_nameBuf, start, len);
  _nameBuf[len] = '\0';

  DataValue dv;
  if (_dataProvider(_nameBuf, expectedKind, dv, false)) {
    switch (expectedKind) {
      case KIND_INT:
        result = (int32_t)dv.intVal;
        break;
      case KIND_UINT:
        result = (int32_t)dv.uintVal;
        break;
      case KIND_FLOAT:
        result = (int32_t)(dv.floatVal * 1000.0f);
        break;
      case KIND_STRING:
        result = (int32_t)hash((char*)dv.stringVal.data);
        break;
      default:
        return false;
    }
    *p = end;
#if ENABLE_DATA_LOGGING && ENABLE_LOGGING
    char buf[32];
    switch (expectedKind) {
      case KIND_INT:
        snprintf(buf, sizeof(buf), "%d", dv.intVal);
        break;
      case KIND_UINT:
        snprintf(buf, sizeof(buf), "%u", dv.uintVal);
        break;
      case KIND_FLOAT:
        snprintf(buf, sizeof(buf), "%.2f", dv.floatVal);
        break;
      case KIND_STRING:
        snprintf(buf, sizeof(buf), "%s", (char*)dv.stringVal.data);
        break;
    }
    logDataAction(_nameBuf, expectedKind, false, buf, 0);
#endif
    return true;
  }
  return false;
}

bool ScriptRunner::parseValue(const char** p, ScriptState& s, int32_t& result, DataKind expectedKind) {
  const char* pos = *p;
  while (*pos == ' ') pos++;

  if (*pos == '$' && pos[1] == 'e' && isDigit(pos[2])) {
    pos += 2;
    uint8_t idx = 0;
    while (isDigit(*pos)) {
      idx = idx * 10 + (*pos - '0');
      pos++;
    }
    if (idx < MAX_EVENT_PARAMS) {
      result = _ctx.eventParams[idx];
      *p = pos;
      return true;
    }
    result = 0;
    *p = pos;
    return true;
  }

  if (*pos == '$') {
    const char* nameStart = pos;
    pos++;
    char type = *pos;

    if ((type == 'v' || type == 'i' || type == 'f' || type == 's' || type == 'a' || type == 'p') && isDigit(*(pos + 1))) {
      pos++;
      uint8_t idx = 0;
      while (isDigit(*pos)) {
        idx = idx * 10 + (*pos - '0');
        pos++;
      }
      *p = pos;
      switch (type) {
        case 'v':
          return parseVarUint(idx, result);
        case 'i':
          return parseVarInt(idx, result);
        case 'f':
          return parseVarFloat(idx, result);
        case 's':
          return parseVarString(idx, result);
        case 'p':
          return parseVarPort(idx, result, s.id);
        default:
          return false;
      }
    }
    *p = nameStart;
    return parseVarData(nameStart, result, p, expectedKind);
  }

  if (*pos == '\'') {
    if (!parseString(&pos, _strBuf)) return false;
    *p = pos;
    if (strlen(_strBuf) == 0) {
      result = 0;
      return true;
    }
    result = (int32_t)hash(_strBuf);
    return true;
  }

  if (isDigit(*pos) || *pos == '.' || *pos == '-') {
    const char* temp = pos;
    bool hasDot = false;
    while (*temp && (*temp != ',' && *temp != ' ' && *temp != '&' && *temp != '|' &&
                     *temp != '=' && *temp != '!' && *temp != '<' && *temp != '>' &&
                     *temp != '+' && *temp != '-' && *temp != '*' && *temp != '/' &&
                     *temp != ')' && *temp != '(')) {
      if (*temp == '.') hasDot = true;
      temp++;
    }
    if (hasDot) {
      float fval = parseFloat(&pos);
      *p = pos;
      result = (int32_t)(fval * 1000.0f);
      return true;
    } else {
      char* end;
      long val = strtol(pos, &end, 10);
      if (end > pos) {
        *p = end;
        result = (int32_t)val;
        return true;
      }
      return false;
    }
  }
  return false;
}

bool ScriptRunner::parseArray(const char** p, uint8_t idx) {
  const char* pos = *p;
  if (*pos != '{') return false;
  pos++;
  uint8_t i = 0;
  while (*pos && *pos != '}' && i < MAX_ARRAY_SIZE) {
    int32_t val = parseInt(&pos);
    _ctx.arrayVars[idx][i] = (uint8_t)val;
    i++;
    if (*pos == ARRAY_SEPARATOR) pos++;
  }
  if (*pos != '}') return false;
  pos++;
  _ctx.arrayLen[idx] = i;
  *p = pos;
  return true;
}

bool ScriptRunner::parseCondition(const char* token, ScriptState& s) {
  const char* p = token;
  bool result = false;
  bool hasResult = false;
  char pendingOp = '\0';

  bool negate = false;
  while (*p == ' ') p++;
  if (*p == '!') {
    negate = true;
    p++;
    while (*p == ' ') p++;
  }

  const char* temp = p;
  bool hasOperator = false;
  while (*temp) {
    if (*temp == '=' || *temp == '!' || *temp == '>' || *temp == '<' ||
        (*temp == '&' && temp[1] == '&') || (*temp == '|' && temp[1] == '|')) {
      if (*temp == '!' && temp == p) {
        temp++;
        continue;
      }
      hasOperator = true;
      break;
    }
    temp++;
  }

  if (!hasOperator) {
    int32_t value = 0;
    const char* start = p;

    if (*p == '\'') {
      char strBuf[MAX_STRING_LEN];
      if (parseString(&p, strBuf)) {
        result = (strlen(strBuf) > 0);
        if (negate) result = !result;
        s.ifResult = result;
        return result;
      }
      return false;
    } else if (*p == '$') {
      const char* p2 = start;
      if (parseValue(&p2, s, value, KIND_INT)) {
        result = (value != 0);
        if (negate) result = !result;
        s.ifResult = result;
        return result;
      }

      p2 = start;
      if (parseValue(&p2, s, value, KIND_UINT)) {
        result = (value != 0);
        if (negate) result = !result;
        s.ifResult = result;
        return result;
      }
      p2 = start;
      if (parseValue(&p2, s, value, KIND_FLOAT)) {
        result = (value != 0);
        if (negate) result = !result;
        s.ifResult = result;
        return result;
      }
      return false;
    } else if (isDigit(*p) || *p == '.') {
      char* end;
      long val = strtol(p, &end, 10);
      if (end > p) {
        result = (val != 0);
        if (negate) result = !result;
        s.ifResult = result;
        return result;
      }
      return false;
    } else {
      return false;
    }
  }

  while (*p && *p != TOKEN_SEPARATOR) {
    int32_t leftVal;
    if (!parseValue(&p, s, leftVal, KIND_INT)) {
      s.ifResult = false;
      return false;
    }
    while (*p == ' ') p++;

    char op1 = *p;
    char op2 = '\0';
    p++;
    if (*p == '=') {
      op2 = '=';
      p++;
    }
    while (*p == ' ') p++;

    int32_t rightVal;
    if (!parseValue(&p, s, rightVal, KIND_INT)) {
      s.ifResult = false;
      return false;
    }

    bool condResult = false;
    bool isLeftString = (leftVal > 0x1000);
    bool isRightString = (rightVal > 0x1000);

    if (isLeftString || isRightString) {
      if (op1 == '=' && op2 == '=')
        condResult = (leftVal == rightVal);
      else if (op1 == '!' && op2 == '=')
        condResult = (leftVal != rightVal);
      else
        condResult = false;
    } else {
      if (op1 == '=' && op2 == '=')
        condResult = (leftVal == rightVal);
      else if (op1 == '!' && op2 == '=')
        condResult = (leftVal != rightVal);
      else if (op1 == '>' && op2 == '\0')
        condResult = (leftVal > rightVal);
      else if (op1 == '<' && op2 == '\0')
        condResult = (leftVal < rightVal);
      else if (op1 == '>' && op2 == '=')
        condResult = (leftVal >= rightVal);
      else if (op1 == '<' && op2 == '=')
        condResult = (leftVal <= rightVal);
    }

    if (!hasResult) {
      result = condResult;
      hasResult = true;
    } else if (pendingOp == '&')
      result = result && condResult;
    else if (pendingOp == '|')
      result = result || condResult;

    while (*p == ' ') p++;
    if (*p == '&' && p[1] == '&') {
      pendingOp = '&';
      p += 2;
    } else if (*p == '|' && p[1] == '|') {
      pendingOp = '|';
      p += 2;
    } else
      break;
    while (*p == ' ') p++;
  }

  if (negate) result = !result;
  s.ifResult = result;
  return result;
}

bool ScriptRunner::handleCall(const Params& params, ScriptState& s) {
  if (params.count < 1) {
    setError("call needs 1 param", s.id, s.pos);
    return false;
  }
  int8_t slot = runScript((uint8_t)atoi(params.values[0]));
  return slot >= 0;
}

bool ScriptRunner::handleWait(const Params& params, ScriptState& s, uint32_t now) {
  if (params.count < 1) {
    setError("wait needs 1 param", s.id, s.pos);
    return false;
  }
  uint32_t duration = parseTime(params.values[0]);
  if (duration > 0) {
    s.inWait = true;
    s.waitUntil = now + duration;
  }
  return true;
}

bool ScriptRunner::handleWhile(const char* params, ScriptState& s) {
  if (s.blockDepth >= MAX_BLOCK_NESTING) {
    setError("block nesting too deep", s.id, s.pos);
    return false;
  }
  strncpy(s.whileConditionBuffer, params, 31);
  s.whileConditionBuffer[31] = '\0';
  s.isWhile = true;
  if (!parseCondition(s.whileConditionBuffer, s)) {
    s.blockStack[s.blockDepth] = BLOCK_WHILE;
    s.blockDepth++;
    s.skipDepth++;
    return true;
  }
  s.loopStartPos = s.pos;
  s.repeatCount = 1;
  s.isInfinite = false;
  s.blockStack[s.blockDepth] = BLOCK_WHILE;
  s.blockDepth++;
  return true;
}

bool ScriptRunner::handleIf(const char* params, ScriptState& s) {
  if (s.skipElse) {
    s.skipDepth++;
    return true;
  }
  if (s.blockDepth >= MAX_BLOCK_NESTING) {
    setError("block nesting too deep", s.id, s.pos);
    return false;
  }
  if (s.ifDepth >= MAX_IF_NESTING) {
    setError("if nesting too deep", s.id, s.pos);
    return false;
  }
  bool condition = parseCondition(params, s);
  s.ifStack[s.ifDepth].result = condition;
  s.ifStack[s.ifDepth].skipElse = !condition;
  s.ifDepth++;
  s.ifResult = condition;
  s.skipElse = !condition;
  s.blockStack[s.blockDepth] = BLOCK_IF;
  s.blockDepth++;
  return true;
}

bool ScriptRunner::handleElse(ScriptState& s) {
  if (s.skipDepth > 0) {
    return true;
  }
  if (s.blockDepth == 0 || s.blockStack[s.blockDepth - 1] != BLOCK_IF) {
    setError("else without if", s.id, s.pos);
    return false;
  }
  if (s.ifDepth == 0) {
    setError("else without if", s.id, s.pos);
    return false;
  }
  uint8_t top = s.ifDepth - 1;
  if (!s.ifStack[top].result) {
    s.ifStack[top].skipElse = false;
    s.skipElse = false;
    s.ifResult = false;
  } else {
    s.ifStack[top].skipElse = true;
    s.skipElse = true;
    s.ifResult = true;
  }
  return true;
}

bool ScriptRunner::handleEnd(ScriptState& s) {
  if (s.skipDepth > 0) {
    s.skipDepth--;
    return true;
  }

  if (s.blockDepth == 0) {
    setError("end without block", s.id, s.pos);
    return false;
  }

  BlockType type = (BlockType)s.blockStack[s.blockDepth - 1];
  s.blockDepth--;

  if (type == BLOCK_WHILE) {
    if (s.isWhile) {
      if (parseCondition(s.whileConditionBuffer, s)) {
        s.pos = s.loopStartPos;
        s.blockStack[s.blockDepth] = BLOCK_WHILE;
        s.blockDepth++;
        return true;
      }
      s.isWhile = false;
      s.repeatCount = 0;
    }
    return true;
  } else if (type == BLOCK_IF) {
    if (s.ifDepth == 0) {
      setError("if depth mismatch", s.id, s.pos);
      return false;
    }
    s.ifDepth--;
    if (s.ifDepth == 0) {
      s.ifResult = false;
      s.skipElse = false;
    } else {
      uint8_t parent = s.ifDepth - 1;
      s.ifResult = s.ifStack[parent].result;
      s.skipElse = s.ifStack[parent].skipElse;
    }
    return true;
  } else if (type == BLOCK_ON) {
    s.inEventHandler = false;
    return true;
  } else {
    setError("unknown block type", s.id, s.pos);
    return false;
  }
}

bool ScriptRunner::processToken(const char* token, ScriptState& s, uint32_t now) {
  if (!token || token[0] == '\0') {
    s.pos++;
    return false;
  }

  if (s.blockDepth > MAX_BLOCK_NESTING) {
    setError("block nesting too deep", s.id, s.pos);
    s.active = false;
    return false;
  }

  if (s.skipElse) {
    if (s.blockDepth == 0 || s.blockStack[s.blockDepth - 1] != BLOCK_IF) {
      s.skipElse = false;
    } else {
      if (strcmp(token, "else") == 0) {
        return handleElse(s);
      }
      if (strcmp(token, "end") == 0) {
        return handleEnd(s);
      }
      const char* colon = strchr(token, ':');
      if (colon) {
        char cmd[16];
        uint16_t len = colon - token;
        if (len > 15) len = 15;
        strncpy(cmd, token, len);
        cmd[len] = '\0';
        if (strcmp(cmd, "if") == 0 || strcmp(cmd, "while") == 0) {
          s.skipDepth++;
          return true;
        }
      }
      if (strncmp(token, "on(", 3) == 0) {
        s.skipDepth++;
        return true;
      }
      return true;
    }
  }

  if (strcmp(token, "return") == 0) {
    if (!s.isHandler) {
      setError("return only allowed in event handler", s.id, s.pos);
      return false;
    }
    s.active = false;
    s.blockDepth = 0;
    s.ifDepth = 0;
    s.skipDepth = 0;
    s.inWait = false;
    s.inEventHandler = false;
    s.hasTempResult = false;
    return true;
  }

  if (strcmp(token, "else") == 0) return handleElse(s);
  if (strcmp(token, "end") == 0) return handleEnd(s);

  const char* colon = strchr(token, ':');
  if (colon) {
    char cmd[16];
    uint16_t len = colon - token;
    if (len > 15) len = 15;
    strncpy(cmd, token, len);
    cmd[len] = '\0';
    const char* params = colon + 1;
    if (strcmp(cmd, "while") == 0) return handleWhile(params, s);
    if (strcmp(cmd, "if") == 0) return handleIf(params, s);
  }

  if (strchr(token, '(')) return processCommand(token, s, now);
  if (token[0] == '$') return handleAssignment(token, s);

  setError("unknown token", token, s.id, s.pos);
  return false;
}

void ScriptRunner::resetScriptState(uint8_t idx) {
  ScriptState& s = _slots[idx];
  s.active = false;
  s.registered = false;
  s.isHandler = false;
  s.isPersistent = false;
  s.id = 0;
  s.script[0] = '\0';
  s.scriptLen = 0;
  s.pos = 0;
  s.startTime = 0;
  s.lastExecutionTime = 0;

  s.blockDepth = 0;
  memset(s.blockStack, 0, sizeof(s.blockStack));
  s.ifDepth = 0;
  memset(s.ifStack, 0, sizeof(s.ifStack));
  s.isWhile = false;
  s.whileConditionBuffer[0] = '\0';
  s.loopStartPos = 0;
  s.repeatCount = 0;
  s.isInfinite = false;
  s.skipElse = false;
  s.skipDepth = 0;
  s.ifResult = false;
  s.inWait = false;
  s.waitUntil = 0;
  s.tempResult = 0;
  s.hasTempResult = false;
  s.inEventHandler = false;
}

int8_t ScriptRunner::findSlotById(uint8_t id) const {
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    if (_slots[i].registered && _slots[i].id == id) return (int8_t)i;
  }
  return -1;
}

int8_t ScriptRunner::findFreeSlot(uint16_t scriptLen) {
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    if (!_slots[i].registered && _slots[i].slotSize >= scriptLen) return (int8_t)i;
  }
  return -1;
}

void ScriptRunner::initSlotPools() {
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    resetScriptState(i);
    _slots[i].slotSize = MAX_SCRIPT_LEN;
  }
  for (uint8_t i = 0; i < 40; i++) _lastPortValues[i] = 0;
}

uint8_t ScriptRunner::readPort(uint8_t pin) {
  uint16_t val = 0;
  if (_portProvider && _portProvider(pin, PORT_READ, val)) {
    if (pin < 40) _lastPortValues[pin] = (uint8_t)val;
    return (uint8_t)val;
  }
  if (pin < 40) return _lastPortValues[pin];
  return 0;
}

void ScriptRunner::writePort(uint8_t pin, uint16_t value, uint8_t slot) {
  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;
  if (pin < 40) _lastPortValues[pin] = (uint8_t)value;
#if ENABLE_PORT_LOGGING && ENABLE_LOGGING
  logPortAction(pin, PORT_WRITE, value, slot);
#endif
  if (_portProvider) {
    _portProvider(pin, PORT_WRITE, value);
    if (_stateChangeProvider) _stateChangeProvider(pin, value);
  }
}

void ScriptRunner::writePortSilent(uint8_t pin, uint16_t value) {
  if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;
  if (pin < 40) _lastPortValues[pin] = (uint8_t)value;
  if (_portProvider) _portProvider(pin, PORT_WRITE, value);
}

bool ScriptRunner::onEvent(uint32_t hash, uint8_t slotId) {
  if (slotId >= MAX_SCRIPTS) return false;
  if (!_slots[slotId].registered) return false;
  for (uint8_t i = 0; i < _eventHandlerCount; i++) {
    if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) return true;
  }
  if (_eventHandlerCount >= MAX_EVENT_HANDLERS) return false;
  _eventHandlers[_eventHandlerCount].hash = hash;
  _eventHandlers[_eventHandlerCount].slotId = slotId;
  _eventHandlers[_eventHandlerCount].active = true;
  _eventHandlerCount++;
  return true;
}

bool ScriptRunner::onEvent(const char* eventName, uint8_t slotId) {
  return onEvent(ScriptRunner::hash(eventName), slotId);
}

void ScriptRunner::emitEvent(uint32_t hash) {
  for (uint8_t i = 0; i < _eventHandlerCount; i++) {
    if (!_eventHandlers[i].active || _eventHandlers[i].hash != hash) continue;
    uint8_t slotId = _eventHandlers[i].slotId;
    if (slotId >= MAX_SCRIPTS) continue;
    if (!_slots[slotId].registered) {
      _eventHandlers[i].active = false;
      continue;
    }
    runScriptFrom(slotId, 0, getSlotLen(slotId));
  }
}

void ScriptRunner::emitEvent(const char* eventName) {
  emitEvent(eventName, 0);
}

void ScriptRunner::emitEvent(const char* eventName, uint8_t paramCount, ...) {
  _ctx.eventParamCount = (paramCount > MAX_EVENT_PARAMS) ? MAX_EVENT_PARAMS : paramCount;

  va_list args;
  va_start(args, paramCount);
  for (uint8_t i = 0; i < _ctx.eventParamCount; i++) {
    _ctx.eventParams[i] = va_arg(args, int32_t);
  }
  va_end(args);

  for (uint8_t i = _ctx.eventParamCount; i < MAX_EVENT_PARAMS; i++) {
    _ctx.eventParams[i] = 0;
  }

#if ENABLE_EVENT_LOGGING && ENABLE_LOGGING
  logEventAction(eventName, _ctx.eventParamCount, _ctx.eventParams);
#endif

  uint32_t hash = ScriptRunner::hash(eventName);
  emitEvent(hash);
}

bool ScriptRunner::removeEventHandler(uint32_t hash) {
  for (uint8_t i = 0; i < _eventHandlerCount; i++) {
    if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) {
      _eventHandlers[i].active = false;
      uint8_t slotId = _eventHandlers[i].slotId;
      if (slotId < MAX_SCRIPTS) resetScriptState(slotId);
      return true;
    }
  }
  return false;
}

void ScriptRunner::clearAllEventHandlers() {
  for (uint8_t i = 0; i < _eventHandlerCount; i++) {
    if (_eventHandlers[i].active) {
      uint8_t slotId = _eventHandlers[i].slotId;
      if (slotId < MAX_SCRIPTS) resetScriptState(slotId);
    }
  }
  _eventHandlerCount = 0;
  for (uint8_t i = 0; i < MAX_EVENT_HANDLERS; i++) {
    _eventHandlers[i].active = false;
    _eventHandlers[i].hash = 0;
    _eventHandlers[i].slotId = 0;
  }
}

bool ScriptRunner::registerScript(uint8_t id, const char* script, bool persistent) {
  uint16_t len = strlen(script);
  if (len >= MAX_SCRIPT_LEN) {
    setError("script too long");
    return false;
  }
  int8_t existing = findSlotById(id);
  if (existing != -1) {
    if (_slots[existing].isHandler) {
      setError("slot is handler", id, 0);
      return false;
    }
    strcpy(_slots[existing].script, script);
    _slots[existing].scriptLen = len;
    _slots[existing].pos = 0;
    _slots[existing].active = false;
    _slots[existing].isHandler = false;
    _slots[existing].isPersistent = persistent;
    _slots[existing].inEventHandler = false;
    return true;
  }
  int8_t slot = findFreeSlot(len);
  if (slot == -1) {
    setError("no free slots");
    return false;
  }
  resetScriptState((uint8_t)slot);
  _slots[slot].registered = true;
  _slots[slot].id = id;
  _slots[slot].isHandler = false;
  _slots[slot].isPersistent = persistent;
  strcpy(_slots[slot].script, script);
  _slots[slot].scriptLen = len;
  _slots[slot].pos = 0;
  _slots[slot].active = false;
  _slots[slot].inEventHandler = false;
  return true;
}

int8_t ScriptRunner::runScript(uint8_t id) {
  int8_t slot = findSlotById(id);
  if (slot == -1 && _loadProvider) {
    char buffer[MAX_SCRIPT_LEN];
    uint16_t len = 0;
    if (_loadProvider(id, buffer, len)) {
      if (registerScript(id, buffer, false)) slot = findSlotById(id);
    }
  }
  if (slot == -1) {
    setError("script not found", id, 0);
    return -1;
  }
  if (_slots[slot].isHandler) {
    setError("cannot run handler", id, 0);
    return -1;
  }
  if (_slots[slot].active) {
#if ENABLE_SCRIPT_LOGGING && ENABLE_LOGGING
    logScriptAction((uint8_t)slot, "already running");
#endif
    return slot;
  }

  _slots[slot].pos = 0;
  _slots[slot].scriptLen = strlen(_slots[slot].script);
  _slots[slot].active = true;
  _slots[slot].startTime = millis();
  _slots[slot].lastExecutionTime = 0;
  _slots[slot].blockDepth = 0;
  _slots[slot].ifDepth = 0;
  _slots[slot].skipDepth = 0;
  _slots[slot].skipElse = false;
  _slots[slot].inWait = false;
  _slots[slot].inEventHandler = false;

#if ENABLE_LOAD_LOGGING && ENABLE_LOGGING
  logLoadAction(id, _slots[slot].scriptLen, true, (uint8_t)slot);
#endif

#if ENABLE_SCRIPT_LOGGING && ENABLE_LOGGING
  logScriptAction((uint8_t)slot, "started");
#endif

  return slot;
}

bool ScriptRunner::runScriptFrom(uint8_t slot, uint16_t offset, uint16_t len) {
  if (slot >= MAX_SCRIPTS) {
    setError("invalid slot");
    return false;
  }
  if (!_slots[slot].registered) {
    setError("slot not registered");
    return false;
  }
  if (_slots[slot].active) _slots[slot].active = false;
  if (offset + len > _slots[slot].slotSize) len = _slots[slot].slotSize - offset;
  _slots[slot].pos = offset;
  _slots[slot].scriptLen = offset + len;
  _slots[slot].active = true;
  _slots[slot].startTime = millis();
  _slots[slot].lastExecutionTime = 0;
  _slots[slot].blockDepth = 0;
  _slots[slot].ifDepth = 0;
  _slots[slot].skipDepth = 0;
  _slots[slot].skipElse = false;
  _slots[slot].inWait = false;
  _slots[slot].inEventHandler = false;
  return true;
}

bool ScriptRunner::stopScript(uint8_t id) {
  int8_t slot = findSlotById(id);
  if (slot == -1) return false;
  _slots[slot].active = false;
  return true;
}

void ScriptRunner::stopAll() {
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) _slots[i].active = false;
}

bool ScriptRunner::removeScript(uint8_t id) {
  int8_t slot = findSlotById(id);
  if (slot == -1) return false;
  if (_slots[slot].active) _slots[slot].active = false;
  resetScriptState((uint8_t)slot);
  for (uint8_t i = 0; i < _eventHandlerCount; i++) {
    if (_eventHandlers[i].active && _eventHandlers[i].slotId == slot) {
      _eventHandlers[i].active = false;
    }
  }
  return true;
}

bool ScriptRunner::isEventId(uint8_t id) const {
  return id >= EVENT_ID_OFFSET;
}

bool ScriptRunner::getNextToken(ScriptState& s, char* token, uint16_t& tokenLen) {
  const char* p = s.script + s.pos;
  while (*p == ' ' || *p == TOKEN_SEPARATOR || *p == '\t' || *p == '\r' || *p == '\n' || *p < 32) {
    p++;
    s.pos++;
    if (s.pos >= s.scriptLen) break;
  }
  if (s.pos >= s.scriptLen) {
    token[0] = '\0';
    tokenLen = 0;
    return false;
  }
  const char* start = p;
  uint8_t parenCount = 0;
  while (*p && (p - s.script) < s.scriptLen) {
    if (*p == '(') parenCount++;
    if (*p == ')') parenCount--;
    if (*p == TOKEN_SEPARATOR && parenCount == 0) break;
    p++;
  }
  tokenLen = p - start;
  if (tokenLen >= MAX_TOKEN_LEN) tokenLen = MAX_TOKEN_LEN - 1;
  strncpy(token, start, tokenLen);
  token[tokenLen] = '\0';
  s.pos = p - s.script;
  return tokenLen > 0 && token[0] != '\0' && token[0] != TOKEN_SEPARATOR;
}

void ScriptRunner::finishScript(ScriptState& s, uint8_t idx) {
  if (s.isWhile && s.blockDepth > 0 && s.blockStack[s.blockDepth - 1] == BLOCK_WHILE) {
    s.isWhile = false;
    s.repeatCount = 0;
    s.blockDepth--;
  }

  uint8_t scriptId = s.id;
  bool wasHandler = s.isHandler;
  bool wasPersistent = s.isPersistent;

  if (!wasHandler && !wasPersistent) {
    resetScriptState(idx);
  } else {
    s.active = false;
  }

  if (_completeCallback) {
    _completeCallback(idx, scriptId);
  }
}

void ScriptRunner::processScript(uint8_t idx, uint32_t now) {
  ScriptState& s = _slots[idx];
  if (!s.active || !s.registered) return;
  if (s.inWait) {
    if (now >= s.waitUntil)
      s.inWait = false;
    else
      return;
  }
  if (now - s.lastExecutionTime < SCRIPT_EXEC_INTERVAL_MS) return;
  s.lastExecutionTime = now;
  if (s.pos >= s.scriptLen) {
    finishScript(s, idx);
    return;
  }
  uint16_t tokenLen = 0;
  if (!getNextToken(s, _tokenBuf, tokenLen)) {
    if (s.pos >= s.scriptLen) finishScript(s, idx);
    return;
  }
  bool ok = processToken(_tokenBuf, s, now);
  if (!ok && s.active) {
    if (!s.inWait && !s.isWhile && !(s.blockDepth > 0 && s.blockStack[s.blockDepth - 1] == BLOCK_IF)) {
      s.active = false;
    }
  }
}

void ScriptRunner::update() {
  uint32_t now = millis();
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) processScript(i, now);
}

bool ScriptRunner::registerFunction(const char* name, ExternalFunction func, void* userData) {
  if (!name || !func) return false;
  if (_extFuncCount >= MAX_EXTERNAL_FUNCTIONS) {
    setError("too many funcs");
    return false;
  }
  for (uint8_t i = 0; i < _extFuncCount; i++) {
    if (_extFuncs[i].active && strcmp(_extFuncs[i].name, name) == 0) {
      _extFuncs[i].func = func;
      _extFuncs[i].userData = userData;
      return true;
    }
  }
  strncpy(_extFuncs[_extFuncCount].name, name, 15);
  _extFuncs[_extFuncCount].name[15] = '\0';
  _extFuncs[_extFuncCount].func = func;
  _extFuncs[_extFuncCount].userData = userData;
  _extFuncs[_extFuncCount].active = true;
  _extFuncCount++;
  return true;
}

bool ScriptRunner::isExternalFunction(const char* name) const {
  if (!name) return false;
  for (uint8_t i = 0; i < _extFuncCount; i++) {
    if (_extFuncs[i].active && strcmp(_extFuncs[i].name, name) == 0) return true;
  }
  return false;
}

bool ScriptRunner::callExternalFunction(const char* name, uint8_t paramCount, const Value* params, Value& result) {
  if (!name) return false;
  for (uint8_t i = 0; i < _extFuncCount; i++) {
    if (_extFuncs[i].active && strcmp(_extFuncs[i].name, name) == 0) {
      return _extFuncs[i].func(paramCount, params, result, _extFuncs[i].userData);
    }
  }
  return false;
}

void ScriptRunner::setScriptCompleteCallback(ScriptCompleteCallback callback) {
  _completeCallback = callback;
}

const char* ScriptRunner::getScript(uint8_t slot) const {
  if (slot >= MAX_SCRIPTS) return nullptr;
  if (!_slots[slot].registered) return nullptr;
  return _slots[slot].script;
}

uint32_t ScriptRunner::getUintVar(uint8_t idx) const {
  if (idx < MAX_UINT_VARS) return _ctx.uintVars[idx];
  return 0;
}

int32_t ScriptRunner::getIntVar(uint8_t idx) const {
  if (idx < MAX_INT_VARS) return _ctx.intVars[idx];
  return 0;
}

float ScriptRunner::getFloatVar(uint8_t idx) const {
  if (idx < MAX_FLOAT_VARS) return (float)_ctx.floatVars[idx];
  return 0.0f;
}

void ScriptRunner::setUintVar(uint8_t idx, uint32_t value) {
  if (idx < MAX_UINT_VARS) _ctx.uintVars[idx] = value;
}

void ScriptRunner::setIntVar(uint8_t idx, int32_t value) {
  if (idx < MAX_INT_VARS) _ctx.intVars[idx] = value;
}

void ScriptRunner::setFloatVar(uint8_t idx, float value) {
  if (idx < MAX_FLOAT_VARS) _ctx.floatVars[idx] = (double)value;
}

uint8_t ScriptRunner::getArrayByte(uint8_t idx, uint8_t pos) const {
  if (idx < MAX_ARRAY_VARS && pos < _ctx.arrayLen[idx]) return _ctx.arrayVars[idx][pos];
  return 0;
}

void ScriptRunner::setArrayByte(uint8_t idx, uint8_t pos, uint8_t value) {
  if (idx < MAX_ARRAY_VARS && pos < MAX_ARRAY_SIZE) {
    _ctx.arrayVars[idx][pos] = value;
    if (pos >= _ctx.arrayLen[idx]) _ctx.arrayLen[idx] = pos + 1;
  }
}

uint8_t ScriptRunner::getArrayLen(uint8_t idx) const {
  if (idx < MAX_ARRAY_VARS) return _ctx.arrayLen[idx];
  return 0;
}

bool ScriptRunner::isRunning(uint8_t id) const {
  int8_t slot = findSlotById(id);
  if (slot == -1) return false;
  return _slots[slot].active;
}

bool ScriptRunner::isBusy() const {
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    if (_slots[i].active) return true;
  }
  return false;
}

bool ScriptRunner::isSlotUsed(uint8_t slot) const {
  if (slot >= MAX_SCRIPTS) return false;
  return _slots[slot].registered;
}

int8_t ScriptRunner::getSlotId(uint8_t slot) const {
  if (slot >= MAX_SCRIPTS) return -1;
  return _slots[slot].registered ? (int8_t)_slots[slot].id : -1;
}

bool ScriptRunner::isSlotActive(uint8_t slot) const {
  if (slot >= MAX_SCRIPTS) return false;
  return _slots[slot].active;
}

bool ScriptRunner::isSlotHandler(uint8_t slot) const {
  if (slot >= MAX_SCRIPTS) return false;
  return _slots[slot].isHandler;
}

uint16_t ScriptRunner::getSlotLen(uint8_t slot) const {
  if (slot >= MAX_SCRIPTS) return 0;
  return _slots[slot].scriptLen;
}

uint8_t ScriptRunner::getTotalSlots() const { return MAX_SCRIPTS; }

uint8_t ScriptRunner::getUsedSlotsCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    if (_slots[i].registered) count++;
  }
  return count;
}

uint8_t ScriptRunner::getFreeSlotsCount() const {
  return MAX_SCRIPTS - getUsedSlotsCount();
}

#if ENABLE_LOGGING

// Изменено: тип action с PortAction на uint8_t
void ScriptRunner::logPortAction(uint8_t gpio, uint8_t action, uint16_t value, uint8_t slot) {
#if ENABLE_PORT_LOGGING
  if (_logProvider) {
    if (action == PORT_WRITE) {
      snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: $p%d <- %d", slot, gpio, value);
    } else {
      snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: $p%d -> %d", slot, gpio, value);
    }
    _logProvider(_logBuf);
  }
#endif
}

void ScriptRunner::logDataAction(const char* id, DataKind kind, bool write, const char* value, uint8_t slot) {
#if ENABLE_DATA_LOGGING
  if (_logProvider && id && value) {
    char kindChar = 0;
    if (kind != KIND_UINT) {
      switch (kind) {
        case KIND_INT:
          kindChar = 'I';
          break;
        case KIND_FLOAT:
          kindChar = 'F';
          break;
        case KIND_STRING:
          kindChar = 'S';
          break;
        default:
          break;
      }
    }

    if (write) {
      if (kindChar) {
        snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: %s <- %s (%c)", slot, id, value, kindChar);
      } else {
        snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: %s <- %s", slot, id, value);
      }
    } else {
      if (kindChar) {
        snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: %s -> %s (%c)", slot, id, value, kindChar);
      } else {
        snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: %s -> %s", slot, id, value);
      }
    }
    _logProvider(_logBuf);
  }
#endif
}

void ScriptRunner::logLoadAction(uint8_t id, uint16_t len, bool cached, uint8_t slot) {
#if ENABLE_LOAD_LOGGING
  if (_logProvider) {
    snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: load size:%d%s", slot, len, cached ? " C" : "");
    _logProvider(_logBuf);
  }
#endif
}

void ScriptRunner::logEventAction(const char* eventName, uint8_t paramCount, const int32_t* params) {
#if ENABLE_EVENT_LOGGING
  if (_logProvider && eventName) {
    if (paramCount == 0) {
      snprintf(_logBuf, sizeof(_logBuf), "EVENT: %s", eventName);
    } else {
      char paramsStr[32] = {0};
      char* p = paramsStr;
      for (uint8_t i = 0; i < paramCount && i < MAX_EVENT_PARAMS; i++) {
        if (i > 0) {
          *p++ = ',';
          *p++ = ' ';
        }
        p += snprintf(p, sizeof(paramsStr) - (p - paramsStr), "%d", params[i]);
      }
      snprintf(_logBuf, sizeof(_logBuf), "EVENT: %s (%s)", eventName, paramsStr);
    }
    _logProvider(_logBuf);
  }
#endif
}

void ScriptRunner::logScriptAction(uint8_t slot, const char* action) {
#if ENABLE_SCRIPT_LOGGING
  if (_logProvider && action) {
    snprintf(_logBuf, sizeof(_logBuf), "INFO[%d]: %s", slot, action);
    _logProvider(_logBuf);
  }
#endif
}

#endif

void ScriptRunner::setError(const char* msg) {
  if (_logProvider) {
    snprintf(_logBuf, sizeof(_logBuf), "ERR: %s", msg);
    _logProvider(_logBuf);
  }
}

void ScriptRunner::setError(const char* msg, uint8_t slot, uint16_t pos) {
  if (_logProvider) {
    if (slot > 0 || pos > 0) {
      snprintf(_logBuf, sizeof(_logBuf), "ERR[%d]: [%d] %s", slot, pos, msg);
    } else {
      snprintf(_logBuf, sizeof(_logBuf), "ERR: %s", msg);
    }
    _logProvider(_logBuf);
  }
}

void ScriptRunner::setError(const char* msg, const char* token, uint8_t slot, uint16_t pos) {
  if (_logProvider) {
    if (slot > 0 || pos > 0) {
      if (token) {
        snprintf(_logBuf, sizeof(_logBuf), "ERR[%d]: [%d] %s '%s'", slot, pos, msg, token);
      } else {
        snprintf(_logBuf, sizeof(_logBuf), "ERR[%d]: [%d] %s", slot, pos, msg);
      }
    } else {
      if (token) {
        snprintf(_logBuf, sizeof(_logBuf), "ERR: %s '%s'", msg, token);
      } else {
        snprintf(_logBuf, sizeof(_logBuf), "ERR: %s", msg);
      }
    }
    _logProvider(_logBuf);
  }
}

void ScriptRunner::setDataProvider(DataProvider provider) { _dataProvider = provider; }
void ScriptRunner::setLogProvider(LogProvider provider) { _logProvider = provider; }
void ScriptRunner::setPortProvider(PortProvider provider) { _portProvider = provider; }
void ScriptRunner::setStateChangeProvider(StateChangeProvider provider) { _stateChangeProvider = provider; }

#ifdef ENABLE_LOAD_CACHE
int8_t ScriptRunner::findInLoadCache(uint8_t id, char* buffer, uint16_t& len) {
  for (uint8_t i = 0; i < LOAD_CACHE_SIZE; i++) {
    if (_loadCache[i].valid && _loadCache[i].id == id) {
      _loadCache[i].lastAccess = millis();
      _loadCache[i].accessCount++;
      strcpy(buffer, _loadCache[i].script);
      len = _loadCache[i].len;
      _loadCacheHits++;
      return (int8_t)i;
    }
  }
  _loadCacheMisses++;
  return -1;
}

void ScriptRunner::addToLoadCache(uint8_t id, const char* script, uint16_t len) {
  for (uint8_t i = 0; i < LOAD_CACHE_SIZE; i++) {
    if (_loadCache[i].valid && _loadCache[i].id == id) {
      _loadCache[i].lastAccess = millis();
      _loadCache[i].accessCount++;
      return;
    }
  }
  int8_t slot = findEmptyLoadSlot();
  if (slot == -1) slot = findLeastUsedSlot();
  if (slot != -1) {
    _loadCache[slot].id = id;
    strcpy(_loadCache[slot].script, script);
    _loadCache[slot].len = len;
    _loadCache[slot].valid = true;
    _loadCache[slot].lastAccess = millis();
    _loadCache[slot].accessCount = 1;
  }
}

int8_t ScriptRunner::findEmptyLoadSlot() const {
  for (uint8_t i = 0; i < LOAD_CACHE_SIZE; i++) {
    if (!_loadCache[i].valid) return (int8_t)i;
  }
  return -1;
}

int8_t ScriptRunner::findLeastUsedSlot() const {
  int8_t least = 0;
  for (uint8_t i = 1; i < LOAD_CACHE_SIZE; i++) {
    if (_loadCache[i].accessCount < _loadCache[least].accessCount) least = (int8_t)i;
  }
  return least;
}

bool ScriptRunner::cachedLoadProviderWrapper(uint8_t id, char* buffer, uint16_t& len) {
  if (!_instance) return false;
  int8_t found = _instance->findInLoadCache(id, buffer, len);
  if (found != -1) return true;
  if (_instance->_originalLoadProvider) {
    bool result = _instance->_originalLoadProvider(id, buffer, len);
    if (result && len > 0) _instance->addToLoadCache(id, buffer, len);
    return result;
  }
  return false;
}

void ScriptRunner::setLoadProvider(LoadProvider provider) {
#ifdef ENABLE_LOAD_CACHE
  _originalLoadProvider = provider;
  _loadProvider = provider ? cachedLoadProviderWrapper : nullptr;
#else
  _loadProvider = provider;
#endif
}
#else
void ScriptRunner::setLoadProvider(LoadProvider provider) {
  _loadProvider = provider;
}
#endif

bool ScriptRunner::handleOn(const Params& params, ScriptState& s, uint32_t now) {
  if (params.count < 1) {
    setError("on needs 1 param", s.id, s.pos);
    return false;
  }
  if (s.inEventHandler) {
    setError("nested event", s.id, s.pos);
    return false;
  }

  const char* rawEventName = params.values[0];
  char cleanEventName[MAX_EVENT_NAME_LEN] = {0};
  const char* eventName = rawEventName;
  uint16_t len = strlen(rawEventName);
  if (len > 2) {
    char first = rawEventName[0];
    char last = rawEventName[len - 1];
    if ((first == '\'' || first == '"') && (last == '\'' || last == '"')) {
      uint16_t cleanLen = len - 2;
      if (cleanLen >= MAX_EVENT_NAME_LEN) cleanLen = MAX_EVENT_NAME_LEN - 1;
      strncpy(cleanEventName, rawEventName + 1, cleanLen);
      cleanEventName[cleanLen] = '\0';
      eventName = cleanEventName;
    }
  }
  uint32_t eventHash = ScriptRunner::hash(eventName);

  for (uint8_t i = 0; i < _eventHandlerCount; i++) {
    if (_eventHandlers[i].active && _eventHandlers[i].hash == eventHash) return true;
  }

  const char* p = s.script + s.pos;
  const char* bodyStart = p;
  uint8_t depth = 0;
  uint16_t bodyLen = 0;

  while (*p && (p - s.script) < s.scriptLen) {
    if (strncmp(p, "on(", 3) == 0) {
      depth++;
      p += 3;
      continue;
    }
    if (strncmp(p, "if:", 3) == 0) {
      depth++;
      p += 3;
      continue;
    }
    if (strncmp(p, "while:", 6) == 0) {
      depth++;
      p += 6;
      continue;
    }
    if (strncmp(p, "end", 3) == 0) {
      if (depth == 0) {
        bodyLen = p - bodyStart;
        p += 3;
        break;
      }
      depth--;
      p += 3;
      continue;
    }
    p++;
  }

  if (bodyLen == 0) {
    setError("empty handler", s.id, s.pos);
    return false;
  }

  strncpy(_handlerBody, bodyStart, bodyLen);
  _handlerBody[bodyLen] = '\0';

  char* start = _handlerBody;
  while (*start == ',' || *start == ' ' || *start == TOKEN_SEPARATOR) start++;
  uint16_t end = strlen(start);
  while (end > 0 && (start[end - 1] == ',' || start[end - 1] == ' ' || start[end - 1] == TOKEN_SEPARATOR)) end--;
  start[end] = '\0';
  if (start != _handlerBody) memmove(_handlerBody, start, end + 1);

  uint16_t writePos = 0;
  for (uint16_t i = 0; _handlerBody[i]; i++) {
    unsigned char c = (unsigned char)_handlerBody[i];
    if (c >= 32 && c <= 126) _handlerBody[writePos++] = c;
  }
  _handlerBody[writePos] = '\0';

  _cleanedBody[0] = '\0';
  bool first = true;
  char* token = strtok(_handlerBody, ";");
  while (token) {
    while (*token == ' ') token++;
    uint16_t tLen = strlen(token);
    while (tLen > 0 && token[tLen - 1] == ' ') {
      token[tLen - 1] = '\0';
      tLen--;
    }
    if (strlen(token) > 0) {
      if (!first) strcat(_cleanedBody, ";");
      strcat(_cleanedBody, token);
      first = false;
    }
    token = strtok(NULL, ";");
  }
  if (strlen(_cleanedBody) == 0) {
    setError("empty body", s.id, s.pos);
    return false;
  }

  uint16_t scriptLen = strlen(_cleanedBody);
  int8_t slot = findFreeSlot(scriptLen);
  if (slot == -1) {
    setError("no free slot", s.id, s.pos);
    return false;
  }

  uint8_t eventId = (uint8_t)(EVENT_ID_OFFSET + (uint8_t)slot);

  s.pos = p - s.script;
  resetScriptState((uint8_t)slot);
  ScriptState& es = _slots[slot];
  es.registered = true;
  es.id = eventId;
  es.isHandler = true;
  es.isPersistent = true;
  strcpy(es.script, _cleanedBody);
  es.scriptLen = scriptLen;
  es.pos = 0;
  es.active = false;
  es.inEventHandler = false;

  if (!onEvent(eventHash, (uint8_t)slot)) {
    resetScriptState((uint8_t)slot);
    return false;
  }
  return true;
}

bool ScriptRunner::handleAssignment(const char* token, ScriptState& s) {
  const char* eq = strchr(token, '=');
  if (!eq) {
    setError("expected '='", token, s.id, s.pos);
    return false;
  }

  const char* left = token;
  char type = left[1];
  const char* right = eq + 1;

  bool isInternal = false;
  uint8_t index = 0;
  const char* p = left + 2;

  if ((type == 'v' || type == 'i' || type == 'f' || type == 's' || type == 'a' || type == 'p') && isDigit(*p)) {
    isInternal = true;
    while (isDigit(*p)) {
      index = index * 10 + (*p - '0');
      p++;
    }
  }

  if (isInternal) {
    if (type == 'p') {
      if (isDigit(*right) || *right == '-') {
        const char* p2 = right;
        int32_t val = parseInt(&p2);
        writePort(index, (uint16_t)val, s.id);
        return true;
      } else if (*right == '$') {
        int32_t val;
        if (parseValue(&right, s, val, KIND_INT)) {
          writePort(index, (uint16_t)val, s.id);
          return true;
        }
        setError("invalid port value", token, s.id, s.pos);
        return false;
      } else {
        uint16_t val = 0;
        if (_portProvider && _portProvider(index, PORT_READ, val)) {
          s.tempResult = (int32_t)val;
          s.hasTempResult = true;
          return true;
        }
        setError("port read failed", s.id, s.pos);
        return false;
      }
    }

    if (type == 'a' && index < MAX_ARRAY_VARS) {
      if (*right == '{') return parseArray(&right, index);
      if (*right == '$' && right[1] == 'a') {
        right += 2;
        uint8_t srcIdx = (uint8_t)parseUint(&right);
        if (srcIdx < MAX_ARRAY_VARS) {
          _ctx.arrayLen[index] = _ctx.arrayLen[srcIdx];
          for (uint8_t i = 0; i < _ctx.arrayLen[index]; i++) {
            _ctx.arrayVars[index][i] = _ctx.arrayVars[srcIdx][i];
          }
          return true;
        }
        setError("invalid array src", s.id, s.pos);
        return false;
      }
      if (*right == '$' && right[1] == 's') {
        right += 2;
        uint8_t srcIdx = (uint8_t)parseUint(&right);
        if (srcIdx < MAX_STRING_VARS) {
          uint16_t len = strlen(_ctx.stringVars[srcIdx]);
          if (len > MAX_ARRAY_SIZE) len = MAX_ARRAY_SIZE;
          for (uint8_t i = 0; i < len; i++) {
            _ctx.arrayVars[index][i] = (uint8_t)_ctx.stringVars[srcIdx][i];
          }
          _ctx.arrayLen[index] = (uint8_t)len;
          return true;
        }
        setError("invalid string src", s.id, s.pos);
        return false;
      }
      setError("array error", s.id, s.pos);
      return false;
    }

    if (type == 's' && index < MAX_STRING_VARS) {
      if (*right == '\'') {
        if (!parseString(&right, _strBuf)) {
          setError("invalid string", s.id, s.pos);
          return false;
        }
        strncpy(_ctx.stringVars[index], _strBuf, MAX_STRING_LEN - 1);
        _ctx.stringVars[index][MAX_STRING_LEN - 1] = '\0';
        return true;
      } else if (strchr(right, '+') != nullptr) {
        const char* p2 = right;
        char leftStr[MAX_STRING_LEN] = {0};
        char rightStr[MAX_STRING_LEN] = {0};

        if (*p2 == '$' && p2[1] == 's') {
          p2 += 2;
          uint8_t leftIdx = (uint8_t)parseUint(&p2);
          if (leftIdx < MAX_STRING_VARS) {
            strncpy(leftStr, _ctx.stringVars[leftIdx], MAX_STRING_LEN - 1);
            leftStr[MAX_STRING_LEN - 1] = '\0';
          }
        } else if (*p2 == '\'') {
          parseString(&p2, leftStr);
        }
        while (*p2 && *p2 != '+') p2++;
        if (*p2 == '+') p2++;

        if (*p2 == '\'') {
          parseString(&p2, rightStr);
        } else if (*p2 == '$' && p2[1] == 'v') {
          p2 += 2;
          uint8_t rightIdx = (uint8_t)parseUint(&p2);
          if (rightIdx < MAX_UINT_VARS) snprintf(rightStr, MAX_STRING_LEN, "%u", _ctx.uintVars[rightIdx]);
        } else if (*p2 == '$' && p2[1] == 'i') {
          p2 += 2;
          uint8_t rightIdx = (uint8_t)parseUint(&p2);
          if (rightIdx < MAX_INT_VARS) snprintf(rightStr, MAX_STRING_LEN, "%d", _ctx.intVars[rightIdx]);
        } else if (*p2 == '$' && p2[1] == 'f') {
          p2 += 2;
          uint8_t rightIdx = (uint8_t)parseUint(&p2);
          if (rightIdx < MAX_FLOAT_VARS) snprintf(rightStr, MAX_STRING_LEN, "%.2f", _ctx.floatVars[rightIdx]);
        } else if (*p2 == '$' && p2[1] == 's') {
          p2 += 2;
          uint8_t rightIdx = (uint8_t)parseUint(&p2);
          if (rightIdx < MAX_STRING_VARS) {
            strncpy(rightStr, _ctx.stringVars[rightIdx], MAX_STRING_LEN - 1);
            rightStr[MAX_STRING_LEN - 1] = '\0';
          }
        } else if (isDigit(*p2) || *p2 == '-') {
          int32_t val = parseInt(&p2);
          snprintf(rightStr, MAX_STRING_LEN, "%d", val);
        }
        char result[MAX_STRING_LEN];
        strncpy(result, leftStr, MAX_STRING_LEN - 1);
        result[MAX_STRING_LEN - 1] = '\0';
        strncat(result, rightStr, MAX_STRING_LEN - strlen(result) - 1);
        strncpy(_ctx.stringVars[index], result, MAX_STRING_LEN - 1);
        _ctx.stringVars[index][MAX_STRING_LEN - 1] = '\0';
        return true;
      } else if (*right == '$' && right[1] == 's') {
        right += 2;
        uint8_t srcIdx = (uint8_t)parseUint(&right);
        if (srcIdx < MAX_STRING_VARS) {
          strncpy(_ctx.stringVars[index], _ctx.stringVars[srcIdx], MAX_STRING_LEN - 1);
          _ctx.stringVars[index][MAX_STRING_LEN - 1] = '\0';
          return true;
        }
        setError("invalid string src", s.id, s.pos);
        return false;
      } else if (s.hasTempResult) {
        snprintf(_strBuf, MAX_STRING_LEN, "%d", s.tempResult);
        strncpy(_ctx.stringVars[index], _strBuf, MAX_STRING_LEN - 1);
        _ctx.stringVars[index][MAX_STRING_LEN - 1] = '\0';
        s.hasTempResult = false;
        return true;
      }
      setError("string error", s.id, s.pos);
      return false;
    }

    if (type == 'v' || type == 'i' || type == 'f') {
      const char* p2 = right;
      int32_t leftVal = 0;
      float leftFloatVal = 0.0f;
      bool hasLeft = false;
      bool leftIsFloat = false;

      if (s.hasTempResult) {
        leftVal = s.tempResult;
        hasLeft = true;
      } else if (*p2 == '$') {
        if (p2[1] == 'f') {
          const char* temp = p2 + 2;
          uint8_t idx = (uint8_t)parseUint(&temp);
          if (idx < MAX_FLOAT_VARS) {
            leftFloatVal = (float)_ctx.floatVars[idx];
            leftIsFloat = true;
            hasLeft = true;
            p2 = temp;
          }
        } else if (p2[1] == 'p') {
          const char* temp = p2 + 2;
          uint8_t gpio = (uint8_t)parseUint(&temp);
          uint16_t val = 0;
          if (_portProvider && _portProvider(gpio, PORT_READ, val)) {
#if ENABLE_PORT_LOGGING && ENABLE_LOGGING
            logPortAction(gpio, PORT_READ, val, s.id);
#endif
            leftVal = (int32_t)val;
            hasLeft = true;
            p2 = temp;
          } else {
            setError("port read failed", s.id, s.pos);
            return false;
          }
        } else {
          DataKind expected = (type == 'v') ? KIND_UINT : (type == 'i') ? KIND_INT
                                                                        : KIND_FLOAT;
          if (parseValue(&p2, s, leftVal, expected)) hasLeft = true;
        }
      } else if (isDigit(*p2) || *p2 == '.' || *p2 == '-') {
        const char* temp = p2;
        bool hasDot = false;
        while (*temp && (*temp != '+' && *temp != '-' && *temp != '*' && *temp != '/' && *temp != '&' && *temp != '|')) {
          if (*temp == '.') hasDot = true;
          temp++;
        }
        if (hasDot || type == 'f') {
          leftIsFloat = true;
          leftFloatVal = parseFloat(&p2);
          hasLeft = true;
        } else {
          leftVal = parseInt(&p2);
          hasLeft = true;
        }
      }

      if (*p2 == '+' || *p2 == '-' || *p2 == '*' || *p2 == '/' || *p2 == '%' ||
          *p2 == '&' || *p2 == '|' || *p2 == '^' || *p2 == '~' ||
          (*p2 == '<' && p2[1] == '<') || (*p2 == '>' && p2[1] == '>')) {
        char op1 = *p2;
        char op2 = '\0';
        if ((op1 == '<' && p2[1] == '<') || (op1 == '>' && p2[1] == '>')) {
          op2 = p2[1];
          p2 += 2;
        } else
          p2++;

        int32_t rightVal = 0;
        float rightFloatVal = 0.0f;
        bool rightIsFloat = false;

        if (*p2 == '$') {
          if (p2[1] == 'f') {
            const char* temp = p2 + 2;
            uint8_t idx = (uint8_t)parseUint(&temp);
            if (idx < MAX_FLOAT_VARS) {
              rightFloatVal = (float)_ctx.floatVars[idx];
              rightIsFloat = true;
              p2 = temp;
            }
          } else {
            DataKind expected = (type == 'v') ? KIND_UINT : (type == 'i') ? KIND_INT
                                                                          : KIND_FLOAT;
            parseValue(&p2, s, rightVal, expected);
          }
        } else if (isDigit(*p2) || *p2 == '.' || *p2 == '-') {
          const char* temp = p2;
          bool hasDot = false;
          while (*temp && (*temp != '+' && *temp != '-' && *temp != '*' && *temp != '/')) {
            if (*temp == '.') hasDot = true;
            temp++;
          }
          if (hasDot || leftIsFloat || type == 'f') {
            rightIsFloat = true;
            rightFloatVal = parseFloat(&p2);
          } else {
            rightVal = parseInt(&p2);
          }
        }

        int32_t resultInt = 0;
        float resultFloat = 0.0f;
        bool resultIsFloat = leftIsFloat || rightIsFloat || type == 'f';

        if (resultIsFloat) {
          float l = leftIsFloat ? leftFloatVal : (float)leftVal;
          float r = rightIsFloat ? rightFloatVal : (float)rightVal;
          switch (op1) {
            case '+':
              resultFloat = l + r;
              break;
            case '-':
              resultFloat = l - r;
              break;
            case '*':
              resultFloat = l * r;
              break;
            case '/':
              if (r == 0) {
                setError("div by zero", s.id, s.pos);
                return false;
              }
              resultFloat = l / r;
              break;
            default:
              setError("unknown op", s.id, s.pos);
              return false;
          }
          resultInt = (int32_t)resultFloat;
        } else {
          switch (op1) {
            case '+':
              resultInt = leftVal + rightVal;
              break;
            case '-':
              resultInt = leftVal - rightVal;
              break;
            case '*':
              resultInt = leftVal * rightVal;
              break;
            case '/':
              if (rightVal == 0) {
                setError("div by zero", s.id, s.pos);
                return false;
              }
              resultInt = leftVal / rightVal;
              break;
            case '%':
              if (rightVal == 0) {
                setError("div by zero", s.id, s.pos);
                return false;
              }
              resultInt = leftVal % rightVal;
              break;
            case '&':
              resultInt = leftVal & rightVal;
              break;
            case '|':
              resultInt = leftVal | rightVal;
              break;
            case '^':
              resultInt = leftVal ^ rightVal;
              break;
            case '~':
              resultInt = (int32_t)(~(uint32_t)leftVal);
              break;
            case '<':
              resultInt = leftVal << (rightVal & 31);
              break;
            case '>':
              resultInt = leftVal >> (rightVal & 31);
              break;
            default:
              setError("unknown op", s.id, s.pos);
              return false;
          }
          resultFloat = (float)resultInt;
        }

        if (type == 'v')
          _ctx.uintVars[index] = resultIsFloat ? (uint32_t)resultFloat : (uint32_t)resultInt;
        else if (type == 'i')
          _ctx.intVars[index] = resultIsFloat ? (int32_t)resultFloat : resultInt;
        else if (type == 'f')
          _ctx.floatVars[index] = resultIsFloat ? (double)resultFloat : (double)resultInt;
        return true;
      }

      if (hasLeft) {
        if (type == 'v')
          _ctx.uintVars[index] = leftIsFloat ? (uint32_t)leftFloatVal : (uint32_t)leftVal;
        else if (type == 'i')
          _ctx.intVars[index] = leftIsFloat ? (int32_t)leftFloatVal : leftVal;
        else if (type == 'f')
          _ctx.floatVars[index] = leftIsFloat ? (double)leftFloatVal : (double)leftVal;
        return true;
      }
      setError("invalid value", token, s.id, s.pos);
      return false;
    }
    setError("invalid var op", token, s.id, s.pos);
    return false;
  }

  if (_dataProvider) {
    const char* leftStart = left;
    const char* leftEnd = leftStart;
    while (*leftEnd && *leftEnd != '=' && *leftEnd != ',' && *leftEnd != ' ') leftEnd++;
    uint16_t nameLen = leftEnd - leftStart;
    if (nameLen > 31) nameLen = 31;
    strncpy(_nameBuf, leftStart, nameLen);
    _nameBuf[nameLen] = '\0';

    if (*right == '\'') {
      const char* p = right;
      if (!parseString(&p, _strBuf)) {
        setError("invalid string", s.id, s.pos);
        return false;
      }
      DataValue dv;
      dv.stringVal.data = (uint8_t*)_strBuf;
      dv.stringVal.len = strlen(_strBuf);
      if (_dataProvider(_nameBuf, KIND_STRING, dv, true)) {
#if ENABLE_DATA_LOGGING && ENABLE_LOGGING
        logDataAction(_nameBuf, KIND_STRING, true, _strBuf, s.id);
#endif
        return true;
      }
      setError("data write failed", s.id, s.pos);
      return false;
    } else if (*right == '$') {
      const char* p = right;
      char varType = p[1];
      p += 2;
      uint8_t varIndex = (uint8_t)parseUint(&p);
      DataKind kind;
      DataValue dv;
      bool success = false;

      if (varType == 'v' && varIndex < MAX_UINT_VARS) {
        kind = KIND_UINT;
        dv.uintVal = _ctx.uintVars[varIndex];
        success = true;
      } else if (varType == 'i' && varIndex < MAX_INT_VARS) {
        kind = KIND_INT;
        dv.intVal = _ctx.intVars[varIndex];
        success = true;
      } else if (varType == 'f' && varIndex < MAX_FLOAT_VARS) {
        kind = KIND_FLOAT;
        dv.floatVal = (float)_ctx.floatVars[varIndex];
        success = true;
      } else if (varType == 's' && varIndex < MAX_STRING_VARS) {
        kind = KIND_STRING;
        dv.stringVal.data = (uint8_t*)_ctx.stringVars[varIndex];
        dv.stringVal.len = strlen(_ctx.stringVars[varIndex]);
        success = true;
      }
      if (success) {
        if (_dataProvider(_nameBuf, kind, dv, true)) {
#if ENABLE_DATA_LOGGING && ENABLE_LOGGING
          char buf[64];
          switch (kind) {
            case KIND_INT:
              snprintf(buf, sizeof(buf), "%d", dv.intVal);
              break;
            case KIND_UINT:
              snprintf(buf, sizeof(buf), "%u", dv.uintVal);
              break;
            case KIND_FLOAT:
              snprintf(buf, sizeof(buf), "%.2f", dv.floatVal);
              break;
            case KIND_STRING:
              snprintf(buf, sizeof(buf), "%s", (char*)dv.stringVal.data);
              break;
          }
          logDataAction(_nameBuf, kind, true, buf, s.id);
#endif
          return true;
        }
      }
      setError("data write failed", s.id, s.pos);
      return false;
    } else if (isDigit(*right) || *right == '.') {
      const char* p = right;
      int32_t value = parseInt(&p);
      DataValue dv;
      dv.intVal = value;
      if (_dataProvider(_nameBuf, KIND_INT, dv, true)) {
#if ENABLE_DATA_LOGGING && ENABLE_LOGGING
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", value);
        logDataAction(_nameBuf, KIND_INT, true, buf, s.id);
#endif
        return true;
      }
      setError("data write failed", s.id, s.pos);
      return false;
    }
    setError("data op error", s.id, s.pos);
    return false;
  }
  setError("unknown var", token, s.id, s.pos);
  return false;
}

bool ScriptRunner::handleLog(const Params& params, ScriptState& s) {
  if (params.count < 1) {
    setError("log needs at least 1 param", s.id, s.pos);
    return false;
  }
  if (!_logProvider) return true;

  char buf[128];
  snprintf(buf, sizeof(buf), "LOG[%d]: ", s.id);
  uint16_t pos = strlen(buf);

  for (uint8_t i = 0; i < params.count && pos < 120; i++) {
    if (i > 0) {
      buf[pos++] = ' ';
      buf[pos] = '\0';
    }

    const char* param = params.values[i];

    if (param[0] == '\'') {
      const char* p = param;
      char strBuf[MAX_STRING_LEN];
      if (parseString(&p, strBuf)) {
        uint16_t len = strlen(strBuf);
        if (pos + len < sizeof(buf) - 1) {
          strcpy(buf + pos, strBuf);
          pos += len;
        }
      }
    } else if (param[0] == '$') {
      char type = param[1];
      const char* p = param + 2;
      uint8_t idx = 0;
      while (isDigit(*p)) {
        idx = idx * 10 + (*p - '0');
        p++;
      }
      bool found = false;

      switch (type) {
        case 's':
          if (idx < MAX_STRING_VARS) {
            uint16_t len = strlen(_ctx.stringVars[idx]);
            if (pos + len < sizeof(buf) - 1) {
              strcpy(buf + pos, _ctx.stringVars[idx]);
              pos += len;
              found = true;
            }
          }
          break;
        case 'v':
          if (idx < MAX_UINT_VARS) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%u", _ctx.uintVars[idx]);
            found = true;
          }
          break;
        case 'i':
          if (idx < MAX_INT_VARS) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", _ctx.intVars[idx]);
            found = true;
          }
          break;
        case 'f':
          if (idx < MAX_FLOAT_VARS) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%.2f", _ctx.floatVars[idx]);
            found = true;
          }
          break;
        case 'a':
          if (idx < MAX_ARRAY_VARS) {
            buf[pos++] = '{';
            for (uint8_t j = 0; j < _ctx.arrayLen[idx]; j++) {
              if (j > 0) {
                buf[pos++] = ',';
                buf[pos] = '\0';
              }
              pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", _ctx.arrayVars[idx][j]);
            }
            buf[pos++] = '}';
            buf[pos] = '\0';
            found = true;
          }
          break;
        case 'p':
          if (idx < 40) {
            uint16_t val = 0;
            if (_portProvider && _portProvider(idx, PORT_READ, val)) {
              pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", val);
              found = true;
            }
          }
          break;
        case 'e': {
          if (idx < MAX_EVENT_PARAMS) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", _ctx.eventParams[idx]);
            found = true;
          }
          break;
        }
        default: {
          int32_t val;
          if (parseValue(&param, s, val, KIND_INT)) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", val);
            found = true;
          } else if (parseValue(&param, s, val, KIND_UINT)) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%u", (uint32_t)val);
            found = true;
          } else if (parseValue(&param, s, val, KIND_FLOAT)) {
            pos += snprintf(buf + pos, sizeof(buf) - pos, "%.2f", (float)val / 1000.0f);
            found = true;
          }
          break;
        }
      }
      if (!found) pos += snprintf(buf + pos, sizeof(buf) - pos, "?");
    } else {
      int32_t val = atoi(param);
      pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", val);
    }
  }

  buf[sizeof(buf) - 1] = '\0';
  _logProvider(buf);
  return true;
}

bool ScriptRunner::handleSet(const Params& params, ScriptState& s) {
  if (params.count < 3) {
    setError("set needs 3 params", s.id, s.pos);
    return false;
  }
  const char* name = params.values[0];
  if (name[0] != '$') {
    setError("set: var expected", s.id, s.pos);
    return false;
  }
  char type = name[1];
  uint8_t idx = (uint8_t)(name[2] - '0');
  int pos = atoi(params.values[1]);
  int val = atoi(params.values[2]);
  if (type == 'a' && idx < MAX_ARRAY_VARS) {
    if (pos >= 0 && pos < MAX_ARRAY_SIZE) {
      _ctx.arrayVars[idx][pos] = (uint8_t)val;
      if (pos >= _ctx.arrayLen[idx]) _ctx.arrayLen[idx] = pos + 1;
      return true;
    }
    setError("array bounds", s.id, s.pos);
    return false;
  } else if (type == 's' && idx < MAX_STRING_VARS) {
    if (pos >= 0 && pos < MAX_STRING_LEN) {
      _ctx.stringVars[idx][pos] = (char)val;
      if (pos >= (int)strlen(_ctx.stringVars[idx])) _ctx.stringVars[idx][pos + 1] = '\0';
      return true;
    }
    setError("string bounds", s.id, s.pos);
    return false;
  }
  setError("set: invalid type", s.id, s.pos);
  return false;
}

bool ScriptRunner::handleGet(const Params& params, ScriptState& s) {
  if (params.count < 2) {
    setError("get needs 2 params", s.id, s.pos);
    return false;
  }
  const char* name = params.values[0];
  if (name[0] != '$') {
    setError("get: var expected", s.id, s.pos);
    return false;
  }
  char type = name[1];
  uint8_t idx = (uint8_t)(name[2] - '0');
  int pos = atoi(params.values[1]);
  uint8_t val = 0;
  if (type == 'a' && idx < MAX_ARRAY_VARS) {
    if (pos >= 0 && pos < _ctx.arrayLen[idx])
      val = _ctx.arrayVars[idx][pos];
    else {
      setError("array bounds", s.id, s.pos);
      return false;
    }
  } else if (type == 's' && idx < MAX_STRING_VARS) {
    uint16_t len = strlen(_ctx.stringVars[idx]);
    if (pos >= 0 && pos < len)
      val = (uint8_t)_ctx.stringVars[idx][pos];
    else {
      setError("string bounds", s.id, s.pos);
      return false;
    }
  } else {
    setError("get: invalid type", s.id, s.pos);
    return false;
  }
  s.tempResult = (int32_t)val;
  s.hasTempResult = true;
  return true;
}

bool ScriptRunner::handleLen(const Params& params, ScriptState& s) {
  if (params.count < 1) {
    setError("len needs 1 param", s.id, s.pos);
    return false;
  }
  const char* name = params.values[0];
  if (name[0] != '$') {
    setError("len: var expected", s.id, s.pos);
    return false;
  }
  char type = name[1];
  uint8_t idx = (uint8_t)(name[2] - '0');
  int len = 0;
  switch (type) {
    case 'a':
      if (idx < MAX_ARRAY_VARS) len = _ctx.arrayLen[idx];
      break;
    case 's':
      if (idx < MAX_STRING_VARS) len = strlen(_ctx.stringVars[idx]);
      break;
    case 'v':
    case 'i':
    case 'f':
      len = 1;
      break;
    default:
      setError("len: invalid type", s.id, s.pos);
      return false;
  }
  s.tempResult = (int32_t)len;
  s.hasTempResult = true;
  return true;
}

bool ScriptRunner::handleChr(const Params& params, ScriptState& s) {
  if (params.count < 1) {
    setError("chr needs 1 param", s.id, s.pos);
    return false;
  }
  s.tempResult = atoi(params.values[0]);
  s.hasTempResult = true;
  return true;
}

bool ScriptRunner::handleOrd(const Params& params, ScriptState& s) {
  if (params.count < 1) {
    setError("ord needs 1 param", s.id, s.pos);
    return false;
  }
  const char* arg = params.values[0];
  uint8_t val = 0;
  if (arg[0] == '\'') {
    const char* p = arg;
    if (!parseString(&p, _strBuf)) {
      setError("invalid string", s.id, s.pos);
      return false;
    }
    val = (uint8_t)_strBuf[0];
  } else if (arg[0] == '$' && arg[1] == 's') {
    uint8_t idx = (uint8_t)(arg[2] - '0');
    if (idx < MAX_STRING_VARS) val = (uint8_t)_ctx.stringVars[idx][0];
  } else if (arg[0] == '$' && arg[1] == 'v') {
    uint8_t idx = (uint8_t)(arg[2] - '0');
    if (idx < MAX_UINT_VARS) val = (uint8_t)_ctx.uintVars[idx];
  } else if (arg[0] == '$' && arg[1] == 'i') {
    uint8_t idx = (uint8_t)(arg[2] - '0');
    if (idx < MAX_INT_VARS) val = (uint8_t)_ctx.intVars[idx];
  } else if (arg[0] == '$' && arg[1] == 'f') {
    uint8_t idx = (uint8_t)(arg[2] - '0');
    if (idx < MAX_FLOAT_VARS) val = (uint8_t)_ctx.floatVars[idx];
  }
  s.tempResult = (int32_t)val;
  s.hasTempResult = true;
  return true;
}

bool ScriptRunner::processCommand(const char* token, ScriptState& s, uint32_t now) {
  const char* open = strchr(token, '(');
  if (!open) {
    setError("expected '('", token, s.id, s.pos);
    return false;
  }
  const char* eq = strchr(token, '=');
  char varName[16] = {0};
  char varType = 0;
  uint8_t varIndex = 0;
  bool hasVar = false;

  if (eq) {
    const char* varStart = token;
    const char* varEnd = eq;
    uint16_t len = varEnd - varStart;
    if (len > 15) len = 15;
    strncpy(varName, varStart, len);
    varName[len] = '\0';
    hasVar = true;
    if (varName[0] == '$') {
      varType = varName[1];
      if (varName[2] >= '0' && varName[2] <= '9') varIndex = varName[2] - '0';
    }
  }

  const char* cmdStart = token;
  if (eq && eq < open) cmdStart = eq + 1;
  char cmd[16];
  uint16_t len = open - cmdStart;
  if (len > 15) len = 15;
  strncpy(cmd, cmdStart, len);
  cmd[len] = '\0';
  Params params = parseParams(token);
  bool result = false;

  if (strcmp(cmd, "call") == 0)
    result = handleCall(params, s);
  else if (strcmp(cmd, "on") == 0)
    result = handleOn(params, s, now);
  else if (strcmp(cmd, "wait") == 0)
    result = handleWait(params, s, now);
  else if (strcmp(cmd, "log") == 0)
    result = handleLog(params, s);
  else if (strcmp(cmd, "set") == 0)
    result = handleSet(params, s);
  else if (strcmp(cmd, "get") == 0)
    result = handleGet(params, s);
  else if (strcmp(cmd, "len") == 0)
    result = handleLen(params, s);
  else if (strcmp(cmd, "chr") == 0)
    result = handleChr(params, s);
  else if (strcmp(cmd, "ord") == 0)
    result = handleOrd(params, s);
  else if (isExternalFunction(cmd)) {
    const char* p = open + 1;
    uint8_t paramCount = 0;

    while (*p && *p != ')' && paramCount < MAX_FUNCTION_PARAMS) {
      while (*p == ' ') p++;
      if (*p == ')' || *p == '\0') break;

      if (*p == '\'') {
        const char* start = p + 1;
        p++;
        while (*p && *p != '\'') p++;
        uint8_t strLen = p - start;
        if (strLen >= MAX_STRING_LEN) strLen = MAX_STRING_LEN - 1;
        strncpy(_funcStrBufs[paramCount], start, strLen);
        _funcStrBufs[paramCount][strLen] = '\0';
        _funcParams[paramCount].type = VAL_STRING;
        _funcParams[paramCount].stringVal.data = _funcStrBufs[paramCount];
        _funcParams[paramCount].stringVal.len = strLen;
        if (*p == '\'') p++;
        paramCount++;
      } else if (*p == '$') {
        char type = p[1];
        p += 2;
        uint8_t idx = 0;
        while (isDigit(*p)) {
          idx = idx * 10 + (*p - '0');
          p++;
        }
        switch (type) {
          case 'v':
            if (idx < MAX_UINT_VARS) {
              _funcParams[paramCount].type = VAL_UINT;
              _funcParams[paramCount].uintVal = _ctx.uintVars[idx];
              paramCount++;
            }
            break;
          case 'i':
            if (idx < MAX_INT_VARS) {
              _funcParams[paramCount].type = VAL_INT;
              _funcParams[paramCount].intVal = _ctx.intVars[idx];
              paramCount++;
            }
            break;
          case 'f':
            if (idx < MAX_FLOAT_VARS) {
              _funcParams[paramCount].type = VAL_FLOAT;
              _funcParams[paramCount].floatVal = (float)_ctx.floatVars[idx];
              paramCount++;
            }
            break;
          case 's':
            if (idx < MAX_STRING_VARS) {
              _funcParams[paramCount].type = VAL_STRING;
              _funcParams[paramCount].stringVal.data = _ctx.stringVars[idx];
              _funcParams[paramCount].stringVal.len = strlen(_ctx.stringVars[idx]);
              paramCount++;
            }
            break;
          case 'a':
            if (idx < MAX_ARRAY_VARS) {
              _funcParams[paramCount].type = VAL_ARRAY;
              _funcParams[paramCount].arrayVal.data = _ctx.arrayVars[idx];
              _funcParams[paramCount].arrayVal.len = _ctx.arrayLen[idx];
              paramCount++;
            }
            break;
          case 'e': {
            if (idx < MAX_EVENT_PARAMS) {
              _funcParams[paramCount].type = VAL_INT;
              _funcParams[paramCount].intVal = _ctx.eventParams[idx];
              paramCount++;
            }
            break;
          }
          default:
            break;
        }
      } else {
        const char* numStart = p;
        const char* temp = p;
        bool hasDot = false;
        while (*temp && *temp != ',' && *temp != ' ' && *temp != ')' && *temp != '(') {
          if (*temp == '.') {
            hasDot = true;
            break;
          }
          temp++;
        }
        p = numStart;
        if (hasDot) {
          float fval = parseFloat(&p);
          _funcParams[paramCount].type = VAL_FLOAT;
          _funcParams[paramCount].floatVal = fval;
        } else {
          int32_t val = parseInt(&p);
          _funcParams[paramCount].type = VAL_INT;
          _funcParams[paramCount].intVal = val;
        }
        paramCount++;
      }
      while (*p == ' ' || *p == ',') p++;
    }

    if (*p != ')') {
      setError("missing ')'", s.id, s.pos);
      return false;
    }

    Value resultVal;
    resultVal.type = VAL_NONE;

    if (callExternalFunction(cmd, paramCount, _funcParams, resultVal)) {
      result = true;
      if (hasVar) {
        bool typeMatch = false;
        switch (resultVal.type) {
          case VAL_INT:
            if (varType == 'i') {
              _ctx.intVars[varIndex] = resultVal.intVal;
              typeMatch = true;
            } else if (varType == 'v') {
              _ctx.uintVars[varIndex] = (uint32_t)resultVal.intVal;
              typeMatch = true;
            } else if (varType == 'f') {
              _ctx.floatVars[varIndex] = (double)resultVal.intVal;
              typeMatch = true;
            }
            break;
          case VAL_UINT:
            if (varType == 'v') {
              _ctx.uintVars[varIndex] = resultVal.uintVal;
              typeMatch = true;
            } else if (varType == 'i') {
              _ctx.intVars[varIndex] = (int32_t)resultVal.uintVal;
              typeMatch = true;
            } else if (varType == 'f') {
              _ctx.floatVars[varIndex] = (double)resultVal.uintVal;
              typeMatch = true;
            }
            break;
          case VAL_FLOAT:
            if (varType == 'f') {
              _ctx.floatVars[varIndex] = (double)resultVal.floatVal;
              typeMatch = true;
            } else if (varType == 'v') {
              _ctx.uintVars[varIndex] = (uint32_t)resultVal.floatVal;
              typeMatch = true;
            } else if (varType == 'i') {
              _ctx.intVars[varIndex] = (int32_t)resultVal.floatVal;
              typeMatch = true;
            }
            break;
          case VAL_STRING:
            if (varType == 's') {
              strncpy(_ctx.stringVars[varIndex], resultVal.stringVal.data, MAX_STRING_LEN - 1);
              _ctx.stringVars[varIndex][MAX_STRING_LEN - 1] = '\0';
              typeMatch = true;
            }
            break;
          case VAL_ARRAY:
            if (varType == 'a') {
              uint8_t len = resultVal.arrayVal.len;
              if (len > MAX_ARRAY_SIZE) len = MAX_ARRAY_SIZE;
              for (uint8_t i = 0; i < len; i++) {
                _ctx.arrayVars[varIndex][i] = resultVal.arrayVal.data[i];
              }
              _ctx.arrayLen[varIndex] = len;
              typeMatch = true;
            }
            break;
          default:
            break;
        }
        if (!typeMatch) {
          setError("type mismatch", s.id, s.pos);
          return false;
        }
      }
      return true;
    } else {
      setError("ext func failed", s.id, s.pos);
      return false;
    }
  } else {
    setError("unknown cmd", cmd, s.id, s.pos);
    return false;
  }

  if (result && s.hasTempResult && hasVar) {
    if (varType == 'v') {
      if (varIndex < MAX_UINT_VARS) _ctx.uintVars[varIndex] = (uint32_t)s.tempResult;
    } else if (varType == 'i') {
      if (varIndex < MAX_INT_VARS) _ctx.intVars[varIndex] = s.tempResult;
    } else if (varType == 'f') {
      if (varIndex < MAX_FLOAT_VARS) _ctx.floatVars[varIndex] = (double)s.tempResult;
    } else if (varType == 's') {
      if (varIndex < MAX_STRING_VARS) {
        char buf[2] = {(char)s.tempResult, '\0'};
        strcpy(_ctx.stringVars[varIndex], buf);
      }
    }
    s.hasTempResult = false;
  }
  return result;
}

ScriptRunner::ScriptRunner()
    : _dataProvider(nullptr), _logProvider(nullptr), _portProvider(nullptr), 
      _stateChangeProvider(nullptr), _loadProvider(nullptr), 
      _eventHandlerCount(0), _extFuncCount(0), _completeCallback(nullptr) {
  _instance = this;
  initSlotPools();

  for (uint8_t i = 0; i < MAX_EXTERNAL_FUNCTIONS; i++) {
    _extFuncs[i].active = false;
    _extFuncs[i].name[0] = '\0';
    _extFuncs[i].func = nullptr;
    _extFuncs[i].userData = nullptr;
  }
  for (uint8_t i = 0; i < MAX_FUNCTION_PARAMS; i++) {
    _funcParams[i].type = VAL_NONE;
    _funcParams[i].intVal = 0;
    _funcStrBufs[i][0] = '\0';
  }

#ifdef ENABLE_LOAD_CACHE
  _loadCacheHits = 0;
  _loadCacheMisses = 0;
  _originalLoadProvider = nullptr;
  for (uint8_t i = 0; i < LOAD_CACHE_SIZE; i++) {
    _loadCache[i].valid = false;
    _loadCache[i].id = 0;
    _loadCache[i].len = 0;
    _loadCache[i].script[0] = '\0';
    _loadCache[i].lastAccess = 0;
    _loadCache[i].accessCount = 0;
  }
#endif

  for (uint8_t i = 0; i < MAX_UINT_VARS; i++) _ctx.uintVars[i] = 0;
  for (uint8_t i = 0; i < MAX_INT_VARS; i++) _ctx.intVars[i] = 0;
  for (uint8_t i = 0; i < MAX_FLOAT_VARS; i++) _ctx.floatVars[i] = 0.0;
  for (uint8_t i = 0; i < MAX_STRING_VARS; i++) _ctx.stringVars[i][0] = '\0';
  for (uint8_t i = 0; i < MAX_ARRAY_VARS; i++) {
    _ctx.arrayLen[i] = 0;
    for (uint8_t j = 0; j < MAX_ARRAY_SIZE; j++) _ctx.arrayVars[i][j] = 0;
  }
  for (uint8_t i = 0; i < MAX_EVENT_HANDLERS; i++) {
    _eventHandlers[i].active = false;
    _eventHandlers[i].hash = 0;
    _eventHandlers[i].slotId = 0;
  }

  for (uint8_t i = 0; i < MAX_EVENT_PARAMS; i++) {
    _ctx.eventParams[i] = 0;
  }
  _ctx.eventParamCount = 0;
}

ScriptRunner::~ScriptRunner() { _instance = nullptr; }

ScriptRunner scriptRunner;