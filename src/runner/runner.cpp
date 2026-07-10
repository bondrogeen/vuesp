#ifdef ARDUINO
#include "./runner.h"
#else
#include "runner.h"
#endif

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>

ScriptRunner* ScriptRunner::_instance = nullptr;

static char stringLiteralPool[8][MAX_STRING_LEN];
static uint8_t stringLiteralIndex = 0;

uint32_t ScriptRunner::hash(const char* str) {
    if (!str) return 0;
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + (uint8_t)*str++;
    }
    return hash;
}

static bool isDigit(char c) {
    return c >= '0' && c <= '9';
}

static bool isAlpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool isAlphaNum(char c) {
    return isAlpha(c) || isDigit(c) || c == '_';
}

uint32_t ScriptRunner::parseUint(const char** p) {
    uint32_t val = 0;
    while (isDigit(**p)) {
        val = val * 10 + (**p - '0');
        (*p)++;
    }
    return val;
}

int32_t ScriptRunner::parseInt(const char** p) {
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

float ScriptRunner::parseFloat(const char** p) {
    char* end;
    float val = strtof(*p, &end);
    if (end > *p) {
        *p = end;
        return val;
    }
    return 0.0f;
}

bool ScriptRunner::parseString(const char** p, char* buf) {
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

uint32_t ScriptRunner::parseTime(const char* str) {
    const char* p = str;
    uint32_t value = 0;
    while (isDigit(*p)) {
        value = value * 10 + (*p - '0');
        p++;
    }
    char unit = *p;
    switch (unit) {
        case 'u': return value;
        case 's': return value * 1000;
        case 'm': return value * 60000;
        case 'h': return value * 3600000;
        default: return value * 1000;
    }
}

Params ScriptRunner::parseParams(const char* str) {
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

bool ScriptRunner::parseVarUint(uint8_t idx, int32_t& result, const char** p, const char* pos) {
    if (idx >= MAX_UINT_VARS) return false;
    result = (int32_t)_ctx.uintVars[idx];
    *p = pos;
    return true;
}

bool ScriptRunner::parseVarInt(uint8_t idx, int32_t& result, const char** p, const char* pos) {
    if (idx >= MAX_INT_VARS) return false;
    result = _ctx.intVars[idx];
    *p = pos;
    return true;
}

bool ScriptRunner::parseVarFloat(uint8_t idx, int32_t& result, const char** p, const char* pos) {
    if (idx >= MAX_FLOAT_VARS) return false;
    float fval = (float)_ctx.floatVars[idx];
    result = (int32_t)(fval * 1000.0f);
    *p = pos;
    return true;
}

bool ScriptRunner::parseVarString(uint8_t idx, int32_t& result, const char** p, const char* pos) {
    if (idx >= MAX_STRING_VARS) return false;
    result = (int32_t)(uintptr_t)_ctx.stringVars[idx];
    *p = pos;
    return true;
}

bool ScriptRunner::parseVarPort(uint8_t idx, int32_t& result, const char** p, const char* pos) {
    uint16_t val = 0;
    if (!_portProvider || !_portProvider(idx, PORT_READ, val)) return false;
    
    #ifdef ENABLE_PROVIDER_LOGGING
    logPortAction(idx, PORT_READ, val);
    #endif
    
    result = (int32_t)val;
    *p = pos;
    return true;
}

bool ScriptRunner::parseVarData(const char* start, int32_t& result, const char** p) {
    if (!_dataProvider) return false;
    const char* end = start;
    while (isAlphaNum(*end)) end++;
    uint16_t len = end - start;
    if (len > 31) len = 31;
    strncpy(_nameBuf, start, len);
    _nameBuf[len] = '\0';

    DataValue dv;
    if (_dataProvider(_nameBuf, KIND_INT, dv, false)) {
        result = (int32_t)dv.intVal;
        *p = end;
        #ifdef ENABLE_PROVIDER_LOGGING
        char buf[32];
        snprintf(buf, sizeof(buf), "%d", dv.intVal);
        logDataAction(_nameBuf, KIND_INT, false, buf);
        #endif
        return true;
    }
    if (_dataProvider(_nameBuf, KIND_UINT, dv, false)) {
        result = (int32_t)dv.uintVal;
        *p = end;
        #ifdef ENABLE_PROVIDER_LOGGING
        char buf[32];
        snprintf(buf, sizeof(buf), "%u", dv.uintVal);
        logDataAction(_nameBuf, KIND_UINT, false, buf);
        #endif
        return true;
    }
    if (_dataProvider(_nameBuf, KIND_FLOAT, dv, false)) {
        result = (int32_t)(dv.floatVal * 1000.0f);
        *p = end;
        #ifdef ENABLE_PROVIDER_LOGGING
        char buf[32];
        snprintf(buf, sizeof(buf), "%.2f", dv.floatVal);
        logDataAction(_nameBuf, KIND_FLOAT, false, buf);
        #endif
        return true;
    }
    return false;
}

bool ScriptRunner::parseValue(const char** p, ScriptState& s, int32_t& result) {
    const char* pos = *p;
    while (*pos == ' ') pos++;

    if (*pos == '$') {
        char type = pos[1];
        pos += 2;
        uint8_t idx = 0;
        while (isDigit(*pos)) {
            idx = idx * 10 + (*pos - '0');
            pos++;
        }

        switch (type) {
            case 'v': return parseVarUint(idx, result, p, pos);
            case 'i': return parseVarInt(idx, result, p, pos);
            case 'f': return parseVarFloat(idx, result, p, pos);
            case 's': return parseVarString(idx, result, p, pos);
            case 'p': return parseVarPort(idx, result, p, pos);
            default: return parseVarData(pos - 2, result, p);
        }
    }

    if (*pos == '\'') {
        if (!parseString(&pos, _strBuf)) return false;
        strcpy(stringLiteralPool[stringLiteralIndex], _strBuf);
        result = (int32_t)(uintptr_t)stringLiteralPool[stringLiteralIndex];
        stringLiteralIndex = (stringLiteralIndex + 1) % 8;
        *p = pos;
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
            result = (int32_t)(fval * 1000.0f);
            *p = pos;
            return true;
        } else {
            char* end;
            long val = strtol(pos, &end, 10);
            if (end > pos) {
                result = (int32_t)val;
                *p = end;
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

    while (*p && *p != TOKEN_SEPARATOR) {
        int32_t leftVal;
        if (!parseValue(&p, s, leftVal)) {
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
        if (!parseValue(&p, s, rightVal)) {
            s.ifResult = false;
            return false;
        }

        bool condResult = false;

        bool isLeftString = (leftVal > 0x1000 && leftVal < 0x50000000);
        bool isRightString = (rightVal > 0x1000 && rightVal < 0x50000000);

        if (isLeftString || isRightString) {
            const char* leftStr = isLeftString ? (const char*)(uintptr_t)leftVal : "";
            const char* rightStr = isRightString ? (const char*)(uintptr_t)rightVal : "";

            if (op1 == '=' && op2 == '=') {
                condResult = (strcmp(leftStr, rightStr) == 0);
            } else if (op1 == '!' && op2 == '=') {
                condResult = (strcmp(leftStr, rightStr) != 0);
            } else {
                condResult = false;
            }
        } else {
            if (op1 == '=' && op2 == '=') condResult = (leftVal == rightVal);
            else if (op1 == '!' && op2 == '=') condResult = (leftVal != rightVal);
            else if (op1 == '>' && op2 == '\0') condResult = (leftVal > rightVal);
            else if (op1 == '<' && op2 == '\0') condResult = (leftVal < rightVal);
            else if (op1 == '>' && op2 == '=') condResult = (leftVal >= rightVal);
            else if (op1 == '<' && op2 == '=') condResult = (leftVal <= rightVal);
        }

        if (!hasResult) {
            result = condResult;
            hasResult = true;
        } else {
            if (pendingOp == '&') {
                result = result && condResult;
            } else if (pendingOp == '|') {
                result = result || condResult;
            }
        }

        while (*p == ' ') p++;

        if (*p == '&' && p[1] == '&') {
            pendingOp = '&';
            p += 2;
        } else if (*p == '|' && p[1] == '|') {
            pendingOp = '|';
            p += 2;
        } else {
            break;
        }

        while (*p == ' ') p++;
    }
    s.ifResult = result;
    return result;
}

bool ScriptRunner::handleCall(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    uint8_t id = (uint8_t)atoi(params.values[0]);
    return runScript(id);
}

bool ScriptRunner::handleWait(const Params& params, ScriptState& s, uint32_t now) {
    if (params.count < 1) return false;
    uint32_t duration = parseTime(params.values[0]);
    if (duration > 0) {
        s.inWait = true;
        s.waitUntil = now + duration;
    }
    return true;
}

bool ScriptRunner::handleWhile(const char* params, ScriptState& s) {
    if (s.inLoop) {
        setError("Nested loops not allowed");
        return false;
    }
    strncpy(s.whileConditionBuffer, params, 31);
    s.whileConditionBuffer[31] = '\0';
    s.isWhile = true;
    bool condition = parseCondition(s.whileConditionBuffer, s);
    if (!condition) return true;
    s.loopStartPos = s.pos;
    s.inLoop = true;
    s.isInfinite = false;
    s.repeatCount = 1;
    return true;
}

bool ScriptRunner::handleIf(const char* params, ScriptState& s) {
    s.ifDepth++;
    s.inIf = true;
    s.skipElse = !parseCondition(params, s);
    return true;
}

bool ScriptRunner::handleElse(ScriptState& s) {
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

bool ScriptRunner::handleEnd(ScriptState& s) {
    if (s.inIf && s.ifDepth > 0) {
        s.ifDepth--;
        if (s.ifDepth == 0) {
            s.inIf = false;
            s.skipElse = false;
            s.ifResult = false;
        }
        return true;
    }
    if (s.inLoop) {
        if (s.isWhile) {
            bool condition = parseCondition(s.whileConditionBuffer, s);
            if (condition) {
                s.pos = s.loopStartPos;
                return true;
            }
            s.inLoop = false;
            s.isWhile = false;
            s.repeatCount = 0;
            return true;
        }
        if (s.repeatCount > 0) {
            s.repeatCount--;
            if (s.repeatCount > 0) {
                s.pos = s.loopStartPos;
                return true;
            }
        }
        s.inLoop = false;
        s.isInfinite = false;
        s.repeatCount = 0;
        return true;
    }
    if (s.inEventHandler) {
        s.inEventHandler = false;
    }
    return true;
}

bool ScriptRunner::handleOn(const Params& params, ScriptState& s, uint32_t now) {
    if (params.count < 1) return false;

    if (s.inEventHandler) {
        setError("Nested events not allowed");
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
        if (_eventHandlers[i].active && _eventHandlers[i].hash == eventHash) {
            return true;
        }
    }

    const char* p = s.script + s.pos;
    const char* bodyStart = p;
    uint8_t depth = 0;
    uint16_t bodyLen = 0;

    while (*p && (p - s.script) < s.scriptLen) {
        if (strncmp(p, "on(", 3) == 0) {
            depth++;
        }
        if (strncmp(p, "end", 3) == 0) {
            if (depth == 0) {
                bodyLen = p - bodyStart;
                break;
            }
            depth--;
        }
        p++;
    }

    if (bodyLen == 0) {
        setError("Empty handler body");
        return false;
    }

    strncpy(_tempBody, bodyStart, bodyLen);
    _tempBody[bodyLen] = '\0';

    char* start = _tempBody;
    while (*start == ',' || *start == ' ' || *start == TOKEN_SEPARATOR) start++;
    uint16_t end = strlen(start);
    while (end > 0 && (start[end - 1] == ',' || start[end - 1] == ' ' || start[end - 1] == TOKEN_SEPARATOR)) end--;
    start[end] = '\0';
    if (start != _tempBody) memmove(_tempBody, start, end + 1);

    uint16_t writePos = 0;
    for (uint16_t i = 0; _tempBody[i]; i++) {
        unsigned char c = (unsigned char)_tempBody[i];
        if (c >= 32 && c <= 126) _tempBody[writePos++] = c;
    }
    _tempBody[writePos] = '\0';

    _resultBuf[0] = '\0';
    bool first = true;
    char* token = strtok(_tempBody, ";");
    while (token) {
        while (*token == ' ') token++;
        uint16_t tLen = strlen(token);
        while (tLen > 0 && token[tLen - 1] == ' ') {
            token[tLen - 1] = '\0';
            tLen--;
        }
        if (strlen(token) > 0) {
            if (!first) strcat(_resultBuf, ";");
            strcat(_resultBuf, token);
            first = false;
        }
        token = strtok(NULL, ";");
    }

    if (strlen(_resultBuf) == 0) {
        setError("Empty body after cleaning");
        return false;
    }

    uint16_t scriptLen = strlen(_resultBuf);
    int slot = findFreeSlot(scriptLen, true);
    if (slot == -1) {
        setError("No free event slot");
        return false;
    }

    uint8_t eventIdx = slot - MAX_SCRIPTS;
    resetEventSlot(eventIdx);
    EventSlot& es = _eventSlots[eventIdx];
    es.registered = true;
    es.id = EVENT_HANDLER_ID_BASE + slot;
    es.isHandler = true;
    es.isPersistent = true;
    strcpy(es.script, _resultBuf);
    es.scriptLen = scriptLen;
    es.pos = 0;
    es.active = false;
    es.inEventHandler = false;

    if (!onEvent(eventHash, (uint8_t)slot)) {
        resetEventSlot(eventIdx);
        return false;
    }

    s.inEventHandler = true;
    s.pos += bodyLen;

    p = s.script + s.pos;
    while (*p && (p - s.script) < s.scriptLen) {
        if (strncmp(p, "end", 3) == 0) {
            s.pos += 3;
            s.inEventHandler = false;
            break;
        }
        p++;
        s.pos++;
    }

    return true;
}

bool ScriptRunner::handleAssignment(const char* token, ScriptState& s) {
    const char* eq = strchr(token, '=');
    if (!eq) return false;

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
                setOutput(index, (uint16_t)val);
                return true;
            } else {
                uint16_t val = 0;
                if (_portProvider && _portProvider(index, PORT_READ, val)) {
                    #ifdef ENABLE_PROVIDER_LOGGING
                    logPortAction(index, PORT_READ, val);
                    #endif
                    s.tempResult = val;
                    s.hasTempResult = true;
                    return true;
                }
                return false;
            }
        }

        if (type == 'a' && index < MAX_ARRAY_VARS) {
            if (*right == '{') {
                return parseArray(&right, index);
            }
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
                    _ctx.arrayLen[index] = len;
                    return true;
                }
            }
            return false;
        }

        if (type == 's' && index < MAX_STRING_VARS) {
            if (*right == '\'') {
                if (!parseString(&right, _strBuf)) return false;
                strcpy(_ctx.stringVars[index], _strBuf);
                return true;
            } else if (strchr(right, '+') != nullptr) {
                const char* p2 = right;
                char leftStr[MAX_STRING_LEN] = {0};
                char rightStr[MAX_STRING_LEN] = {0};

                if (*p2 == '$' && p2[1] == 's') {
                    p2 += 2;
                    uint8_t leftIdx = (uint8_t)parseUint(&p2);
                    if (leftIdx < MAX_STRING_VARS) {
                        strcpy(leftStr, _ctx.stringVars[leftIdx]);
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
                    if (rightIdx < MAX_UINT_VARS) {
                        snprintf(rightStr, MAX_STRING_LEN, "%u", _ctx.uintVars[rightIdx]);
                    }
                } else if (*p2 == '$' && p2[1] == 'i') {
                    p2 += 2;
                    uint8_t rightIdx = (uint8_t)parseUint(&p2);
                    if (rightIdx < MAX_INT_VARS) {
                        snprintf(rightStr, MAX_STRING_LEN, "%d", _ctx.intVars[rightIdx]);
                    }
                } else if (*p2 == '$' && p2[1] == 'f') {
                    p2 += 2;
                    uint8_t rightIdx = (uint8_t)parseUint(&p2);
                    if (rightIdx < MAX_FLOAT_VARS) {
                        snprintf(rightStr, MAX_STRING_LEN, "%.2f", _ctx.floatVars[rightIdx]);
                    }
                } else if (*p2 == '$' && p2[1] == 's') {
                    p2 += 2;
                    uint8_t rightIdx = (uint8_t)parseUint(&p2);
                    if (rightIdx < MAX_STRING_VARS) {
                        strcpy(rightStr, _ctx.stringVars[rightIdx]);
                    }
                } else if (isDigit(*p2) || *p2 == '-') {
                    int32_t val = parseInt(&p2);
                    snprintf(rightStr, MAX_STRING_LEN, "%d", val);
                }

                char result[MAX_STRING_LEN];
                strcpy(result, leftStr);
                strcat(result, rightStr);
                strcpy(_ctx.stringVars[index], result);
                return true;
            } else if (*right == '$' && right[1] == 's') {
                right += 2;
                uint8_t srcIdx = (uint8_t)parseUint(&right);
                if (srcIdx < MAX_STRING_VARS) {
                    strcpy(_ctx.stringVars[index], _ctx.stringVars[srcIdx]);
                    return true;
                }
            } else if (s.hasTempResult) {
                snprintf(_strBuf, MAX_STRING_LEN, "%d", s.tempResult);
                strcpy(_ctx.stringVars[index], _strBuf);
                s.hasTempResult = false;
                return true;
            }
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
                        #ifdef ENABLE_PROVIDER_LOGGING
                        logPortAction(gpio, PORT_READ, val);
                        #endif
                        leftVal = (int32_t)val;
                        hasLeft = true;
                        p2 = temp;
                    } else {
                        return false;
                    }
                } else {
                    if (parseValue(&p2, s, leftVal)) {
                        hasLeft = true;
                    }
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
                } else {
                    p2++;
                }

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
                        parseValue(&p2, s, rightVal);
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
                        case '+': resultFloat = l + r; break;
                        case '-': resultFloat = l - r; break;
                        case '*': resultFloat = l * r; break;
                        case '/': 
                            if (r == 0) return false;
                            resultFloat = l / r; 
                            break;
                        default: return false;
                    }
                    resultInt = (int32_t)resultFloat;
                } else {
                    switch (op1) {
                        case '+': resultInt = leftVal + rightVal; break;
                        case '-': resultInt = leftVal - rightVal; break;
                        case '*': resultInt = leftVal * rightVal; break;
                        case '/': 
                            if (rightVal == 0) return false;
                            resultInt = leftVal / rightVal; 
                            break;
                        case '%': 
                            if (rightVal == 0) return false;
                            resultInt = leftVal % rightVal; 
                            break;
                        case '&': resultInt = leftVal & rightVal; break;
                        case '|': resultInt = leftVal | rightVal; break;
                        case '^': resultInt = leftVal ^ rightVal; break;
                        case '~': resultInt = (int32_t)(~(uint32_t)leftVal); break;
                        case '<': resultInt = leftVal << (rightVal & 31); break;
                        case '>': resultInt = leftVal >> (rightVal & 31); break;
                        default: return false;
                    }
                    resultFloat = (float)resultInt;
                }

                if (type == 'v') {
                    _ctx.uintVars[index] = resultIsFloat ? (uint32_t)resultFloat : (uint32_t)resultInt;
                } else if (type == 'i') {
                    _ctx.intVars[index] = resultIsFloat ? (int32_t)resultFloat : resultInt;
                } else if (type == 'f') {
                    _ctx.floatVars[index] = resultIsFloat ? (double)resultFloat : (double)resultInt;
                }
                return true;
            }

            if (hasLeft) {
                if (type == 'v') {
                    _ctx.uintVars[index] = leftIsFloat ? (uint32_t)leftFloatVal : (uint32_t)leftVal;
                } else if (type == 'i') {
                    _ctx.intVars[index] = leftIsFloat ? (int32_t)leftFloatVal : leftVal;
                } else if (type == 'f') {
                    _ctx.floatVars[index] = leftIsFloat ? (double)leftFloatVal : (double)leftVal;
                }
                return true;
            }
        }

        return false;
    }

    if (_dataProvider) {
        const char* leftStart = left;
        const char* leftEnd = left;
        while (*leftEnd && *leftEnd != '=' && *leftEnd != ',' && *leftEnd != ' ') {
            leftEnd++;
        }
        uint16_t nameLen = leftEnd - leftStart;
        if (nameLen > 31) nameLen = 31;
        strncpy(_nameBuf, leftStart, nameLen);
        _nameBuf[nameLen] = '\0';

        if (*right == '\'') {
            const char* p = right;
            if (!parseString(&p, _strBuf)) return false;
            DataValue dv;
            dv.stringVal.data = (uint8_t*)_strBuf;
            dv.stringVal.len = strlen(_strBuf);
            if (_dataProvider(_nameBuf, KIND_STRING, dv, true)) {
                #ifdef ENABLE_PROVIDER_LOGGING
                logDataAction(_nameBuf, KIND_STRING, true, _strBuf);
                #endif
                return true;
            }
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
                kind = KIND_INT;
                dv.intVal = (int32_t)_ctx.uintVars[varIndex];
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
                    #ifdef ENABLE_PROVIDER_LOGGING
                    char buf[64];
                    switch (kind) {
                        case KIND_INT: snprintf(buf, sizeof(buf), "%d", dv.intVal); break;
                        case KIND_UINT: snprintf(buf, sizeof(buf), "%u", dv.uintVal); break;
                        case KIND_FLOAT: snprintf(buf, sizeof(buf), "%.2f", dv.floatVal); break;
                        case KIND_STRING: snprintf(buf, sizeof(buf), "%s", (char*)dv.stringVal.data); break;
                    }
                    logDataAction(_nameBuf, kind, true, buf);
                    #endif
                    return true;
                }
            }
            return false;
        } else if (isDigit(*right) || *right == '.') {
            const char* p = right;
            int32_t value = parseInt(&p);
            DataValue dv;
            dv.intVal = value;
            if (_dataProvider(_nameBuf, KIND_INT, dv, true)) {
                #ifdef ENABLE_PROVIDER_LOGGING
                char buf[32];
                snprintf(buf, sizeof(buf), "%d", value);
                logDataAction(_nameBuf, KIND_INT, true, buf);
                #endif
                return true;
            }
            return false;
        }
        return false;
    }

    return false;
}

bool ScriptRunner::handleSet(const Params& params, ScriptState& s) {
    if (params.count < 3) return false;
    const char* name = params.values[0];
    if (name[0] != '$') return false;
    char type = name[1];
    uint8_t idx = (uint8_t)(name[2] - '0');
    int pos = atoi(params.values[1]);
    int val = atoi(params.values[2]);
    if (type == 'a' && idx < MAX_ARRAY_VARS) {
        if (pos >= 0 && pos < MAX_ARRAY_SIZE) {
            _ctx.arrayVars[idx][pos] = (uint8_t)val;
            if (pos >= _ctx.arrayLen[idx]) {
                _ctx.arrayLen[idx] = pos + 1;
            }
            return true;
        }
    } else if (type == 's' && idx < MAX_STRING_VARS) {
        if (pos >= 0 && pos < MAX_STRING_LEN) {
            _ctx.stringVars[idx][pos] = (char)val;
            if (pos >= (int)strlen(_ctx.stringVars[idx])) {
                _ctx.stringVars[idx][pos + 1] = '\0';
            }
            return true;
        }
    }
    return false;
}

bool ScriptRunner::handleGet(const Params& params, ScriptState& s) {
    if (params.count < 2) return false;
    const char* name = params.values[0];
    if (name[0] != '$') return false;
    char type = name[1];
    uint8_t idx = (uint8_t)(name[2] - '0');
    int pos = atoi(params.values[1]);
    uint8_t val = 0;
    if (type == 'a' && idx < MAX_ARRAY_VARS) {
        if (pos >= 0 && pos < _ctx.arrayLen[idx]) {
            val = _ctx.arrayVars[idx][pos];
        }
    } else if (type == 's' && idx < MAX_STRING_VARS) {
        uint16_t len = strlen(_ctx.stringVars[idx]);
        if (pos >= 0 && pos < len) {
            val = (uint8_t)_ctx.stringVars[idx][pos];
        }
    } else {
        return false;
    }
    s.tempResult = val;
    s.hasTempResult = true;
    return true;
}

bool ScriptRunner::handleLen(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    const char* name = params.values[0];
    if (name[0] != '$') return false;
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
            return false;
    }
    s.tempResult = len;
    s.hasTempResult = true;
    return true;
}

bool ScriptRunner::handleChr(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    int val = atoi(params.values[0]);
    s.tempResult = val;
    s.hasTempResult = true;
    return true;
}

bool ScriptRunner::handleOrd(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    const char* arg = params.values[0];
    uint8_t val = 0;
    if (arg[0] == '\'') {
        const char* p = arg;
        if (!parseString(&p, _strBuf)) return false;
        val = (uint8_t)_strBuf[0];
    } else if (arg[0] == '$' && arg[1] == 's') {
        uint8_t idx = (uint8_t)(arg[2] - '0');
        if (idx < MAX_STRING_VARS) {
            val = (uint8_t)_ctx.stringVars[idx][0];
        }
    } else if (arg[0] == '$' && arg[1] == 'v') {
        uint8_t idx = (uint8_t)(arg[2] - '0');
        if (idx < MAX_UINT_VARS) {
            val = (uint8_t)_ctx.uintVars[idx];
        }
    } else if (arg[0] == '$' && arg[1] == 'i') {
        uint8_t idx = (uint8_t)(arg[2] - '0');
        if (idx < MAX_INT_VARS) {
            val = (uint8_t)_ctx.intVars[idx];
        }
    } else if (arg[0] == '$' && arg[1] == 'f') {
        uint8_t idx = (uint8_t)(arg[2] - '0');
        if (idx < MAX_FLOAT_VARS) {
            val = (uint8_t)_ctx.floatVars[idx];
        }
    }
    s.tempResult = val;
    s.hasTempResult = true;
    return true;
}

bool ScriptRunner::processCommand(const char* token, ScriptState& s, uint32_t now) {
    const char* open = strchr(token, '(');
    if (!open) return false;
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
            if (varName[2] >= '0' && varName[2] <= '9') {
                varIndex = varName[2] - '0';
            }
        }
    }
    const char* cmdStart = token;
    if (eq && eq < open) {
        cmdStart = eq + 1;
    }
    char cmd[16];
    uint16_t len = open - cmdStart;
    if (len > 15) len = 15;
    strncpy(cmd, cmdStart, len);
    cmd[len] = '\0';
    Params params = parseParams(token);
    bool result = false;
    if (strcmp(cmd, "call") == 0) result = handleCall(params, s);
    else if (strcmp(cmd, "on") == 0) result = handleOn(params, s, now);
    else if (strcmp(cmd, "wait") == 0) result = handleWait(params, s, now);
    else if (strcmp(cmd, "set") == 0) result = handleSet(params, s);
    else if (strcmp(cmd, "get") == 0) result = handleGet(params, s);
    else if (strcmp(cmd, "len") == 0) result = handleLen(params, s);
    else if (strcmp(cmd, "chr") == 0) result = handleChr(params, s);
    else if (strcmp(cmd, "ord") == 0) result = handleOrd(params, s);
    if (result && s.hasTempResult && hasVar) {
        if (varType == 'v') {
            if (varIndex < MAX_UINT_VARS) {
                _ctx.uintVars[varIndex] = (uint32_t)s.tempResult;
            }
        } else if (varType == 'i') {
            if (varIndex < MAX_INT_VARS) {
                _ctx.intVars[varIndex] = s.tempResult;
            }
        } else if (varType == 'f') {
            if (varIndex < MAX_FLOAT_VARS) {
                _ctx.floatVars[varIndex] = (double)s.tempResult;
            }
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

bool ScriptRunner::processToken(const char* token, ScriptState& s, uint32_t now) {
    if (!token || token[0] == '\0') {
        s.pos++;
        return false;
    }
    if (s.inIf && s.skipElse) {
        if (strcmp(token, "end") == 0) return handleEnd(s);
        if (strcmp(token, "else") == 0) return handleElse(s);
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
    if (strchr(token, '(')) {
        return processCommand(token, s, now);
    }
    if (token[0] == '$') return handleAssignment(token, s);
    s.pos++;
    return false;
}

void ScriptRunner::resetScriptState(int idx) {
    ScriptState& s = _slots[idx];
    s.active = false;
    s.registered = false;
    s.inEventHandler = false;
    s.isHandler = false;
    s.isPersistent = false;
    s.id = 0;
    s.script[0] = '\0';
    s.scriptLen = 0;
    s.pos = 0;
    s.startTime = 0;
    s.lastExecutionTime = 0;
    s.inLoop = false;
    s.isInfinite = false;
    s.repeatCount = 0;
    s.loopStartPos = 0;
    s.inIf = false;
    s.ifResult = false;
    s.skipElse = false;
    s.ifDepth = 0;
    s.inWait = false;
    s.waitUntil = 0;
    s.tempResult = 0;
    s.hasTempResult = false;
    s.isWhile = false;
    s.whileConditionBuffer[0] = '\0';
}

void ScriptRunner::resetEventSlot(int idx) {
    EventSlot& s = _eventSlots[idx];
    s.active = false;
    s.registered = false;
    s.inEventHandler = false;
    s.isHandler = false;
    s.isPersistent = false;
    s.id = 0;
    s.script[0] = '\0';
    s.scriptLen = 0;
    s.pos = 0;
    s.startTime = 0;
    s.lastExecutionTime = 0;
    s.inWait = false;
    s.waitUntil = 0;
    s.tempResult = 0;
    s.hasTempResult = false;
}

int ScriptRunner::findSlotById(uint8_t id) const {
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        if (_slots[i].registered && _slots[i].id == id) {
            return i;
        }
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        if (_eventSlots[i].registered && _eventSlots[i].id == id) {
            return MAX_SCRIPTS + i;
        }
    }
    return -1;
}

int ScriptRunner::findFreeSlot(uint16_t scriptLen, bool isHandler) {
    if (isHandler) {
        for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
            if (!_eventSlots[i].registered && _eventSlots[i].slotSize >= scriptLen) {
                return MAX_SCRIPTS + i;
            }
        }
    } else {
        for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
            if (!_slots[i].registered && _slots[i].slotSize >= scriptLen) {
                return i;
            }
        }
    }
    return -1;
}

void ScriptRunner::initSlotPools() {
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        resetScriptState(i);
        _slots[i].slotSize = MAX_SCRIPT_LEN;
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        resetEventSlot(i);
        _eventSlots[i].slotSize = MAX_EVENT_LEN;
    }
}

bool ScriptRunner::onEvent(uint32_t hash, uint8_t slotId) {
    if (slotId >= TOTAL_SLOTS) {
        return false;
    }

    if (slotId < MAX_SCRIPTS) {
        if (!_slots[slotId].registered) {
            return false;
        }
    } else {
        uint8_t idx = slotId - MAX_SCRIPTS;
        if (!_eventSlots[idx].registered) {
            return false;
        }
    }

    for (uint8_t i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) {
            return true;
        }
    }

    if (_eventHandlerCount >= MAX_EVENT_HANDLERS) {
        return false;
    }

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
    uint8_t calledCount = 0;

    for (uint8_t i = 0; i < _eventHandlerCount; i++) {
        if (!_eventHandlers[i].active || _eventHandlers[i].hash != hash) continue;

        uint8_t slotId = _eventHandlers[i].slotId;
        bool slotValid = false;

        if (slotId < MAX_SCRIPTS) {
            slotValid = _slots[slotId].registered;
        } else if (slotId < TOTAL_SLOTS) {
            uint8_t idx = slotId - MAX_SCRIPTS;
            slotValid = _eventSlots[idx].registered;
        }

        if (!slotValid) {
            _eventHandlers[i].active = false;
            continue;
        }

        if (runScriptFrom(slotId, 0, getSlotLen(slotId))) {
            calledCount++;
        }
    }
}

void ScriptRunner::emitEvent(const char* eventName) {
    emitEvent(ScriptRunner::hash(eventName));
}

bool ScriptRunner::removeEventHandler(uint32_t hash) {
    for (uint8_t i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) {
            _eventHandlers[i].active = false;
            uint8_t slotId = _eventHandlers[i].slotId;
            if (slotId < MAX_SCRIPTS) {
                resetScriptState(slotId);
            } else if (slotId < TOTAL_SLOTS) {
                resetEventSlot(slotId - MAX_SCRIPTS);
            }
            return true;
        }
    }
    return false;
}

void ScriptRunner::clearAllEventHandlers() {
    for (uint8_t i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active) {
            uint8_t slotId = _eventHandlers[i].slotId;
            if (slotId < MAX_SCRIPTS) {
                resetScriptState(slotId);
            } else if (slotId < TOTAL_SLOTS) {
                resetEventSlot(slotId - MAX_SCRIPTS);
            }
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
    if (id >= EVENT_HANDLER_ID_BASE) {
        return false;
    }

    uint16_t len = strlen(script);
    if (len >= MAX_SCRIPT_LEN) {
        return false;
    }

    int existing = findSlotById(id);
    if (existing != -1) {
        if (existing < MAX_SCRIPTS) {
            if (_slots[existing].isHandler) {
                return false;
            }
            strcpy(_slots[existing].script, script);
            _slots[existing].scriptLen = len;
            _slots[existing].pos = 0;
            _slots[existing].active = false;
            _slots[existing].inEventHandler = false;
            _slots[existing].isHandler = false;
            _slots[existing].isPersistent = persistent;
        } else {
            uint8_t eventIdx = existing - MAX_SCRIPTS;
            if (len >= MAX_EVENT_LEN) {
                return false;
            }
            strcpy(_eventSlots[eventIdx].script, script);
            _eventSlots[eventIdx].scriptLen = len;
            _eventSlots[eventIdx].pos = 0;
            _eventSlots[eventIdx].active = false;
            _eventSlots[eventIdx].inEventHandler = false;
            _eventSlots[eventIdx].isHandler = true;
            _eventSlots[eventIdx].isPersistent = true;
        }
        return true;
    }

    int slot = findFreeSlot(len, false);
    if (slot == -1) {
        return false;
    }

    resetScriptState(slot);
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

bool ScriptRunner::runScript(uint8_t id) {
    if (id >= EVENT_HANDLER_ID_BASE) {
        return false;
    }

    int slot = findSlotById(id);
    if (slot != -1) {
        if (slot < MAX_SCRIPTS) {
            if (_slots[slot].isHandler) {
                return false;
            }
            if (_slots[slot].active) {
                _slots[slot].active = false;
            }
            _slots[slot].pos = 0;
            _slots[slot].scriptLen = strlen(_slots[slot].script);
            _slots[slot].active = true;
            _slots[slot].startTime = millis();
            _slots[slot].lastExecutionTime = 0;
            _slots[slot].inLoop = false;
            _slots[slot].inIf = false;
            _slots[slot].inWait = false;
            _slots[slot].inEventHandler = false;
            
            #ifdef ENABLE_PROVIDER_LOGGING
            logLoadAction(id, _slots[slot].scriptLen, true);
            #endif
            return true;
        } else {
            uint8_t eventIdx = slot - MAX_SCRIPTS;
            if (_eventSlots[eventIdx].isHandler) {
                return false;
            }
            if (_eventSlots[eventIdx].active) {
                _eventSlots[eventIdx].active = false;
            }
            _eventSlots[eventIdx].pos = 0;
            _eventSlots[eventIdx].scriptLen = strlen(_eventSlots[eventIdx].script);
            _eventSlots[eventIdx].active = true;
            _eventSlots[eventIdx].startTime = millis();
            _eventSlots[eventIdx].lastExecutionTime = 0;
            _eventSlots[eventIdx].inWait = false;
            _eventSlots[eventIdx].inEventHandler = false;
            
            #ifdef ENABLE_PROVIDER_LOGGING
            logLoadAction(id, _eventSlots[eventIdx].scriptLen, true);
            #endif
            return true;
        }
    }

    #ifdef ENABLE_LOAD_CACHE
    if (_loadProvider) {
        char buffer[MAX_SCRIPT_LEN];
        uint16_t len = 0;
        if (_loadProvider(id, buffer, len)) {
            if (len > 0 && len < MAX_SCRIPT_LEN) {
                buffer[len] = '\0';
                if (registerScript(id, buffer, true)) {
                    return runScript(id);
                }
            }
        }
    }
    #else
    if (_loadProvider) {
        char buffer[MAX_SCRIPT_LEN];
        uint16_t len = 0;
        if (_loadProvider(id, buffer, len)) {
            if (len > 0 && len < MAX_SCRIPT_LEN) {
                buffer[len] = '\0';
                if (registerScript(id, buffer, true)) {
                    return runScript(id);
                }
            }
        }
    }
    #endif

    return false;
}

bool ScriptRunner::runScriptFrom(uint8_t slot, uint16_t offset, uint16_t len) {
    if (slot >= TOTAL_SLOTS) {
        return false;
    }

    if (slot < MAX_SCRIPTS) {
        if (!_slots[slot].registered) {
            return false;
        }
        if (_slots[slot].active) {
            _slots[slot].active = false;
        }
        if (offset + len > MAX_SCRIPT_LEN) len = MAX_SCRIPT_LEN - offset;
        _slots[slot].pos = offset;
        _slots[slot].scriptLen = offset + len;
        _slots[slot].active = true;
        _slots[slot].startTime = millis();
        _slots[slot].lastExecutionTime = 0;
        _slots[slot].inLoop = false;
        _slots[slot].inIf = false;
        _slots[slot].inWait = false;
        _slots[slot].inEventHandler = false;
        return true;
    } else {
        uint8_t eventIdx = slot - MAX_SCRIPTS;
        if (!_eventSlots[eventIdx].registered) {
            return false;
        }
        if (_eventSlots[eventIdx].active) {
            _eventSlots[eventIdx].active = false;
        }
        if (offset + len > MAX_EVENT_LEN) len = MAX_EVENT_LEN - offset;
        _eventSlots[eventIdx].pos = offset;
        _eventSlots[eventIdx].scriptLen = offset + len;
        _eventSlots[eventIdx].active = true;
        _eventSlots[eventIdx].startTime = millis();
        _eventSlots[eventIdx].lastExecutionTime = 0;
        _eventSlots[eventIdx].inWait = false;
        _eventSlots[eventIdx].inEventHandler = false;
        return true;
    }
}

bool ScriptRunner::stopScript(uint8_t id) {
    int slot = findSlotById(id);
    if (slot == -1) return false;
    if (slot < MAX_SCRIPTS) {
        _slots[slot].active = false;
    } else {
        uint8_t eventIdx = slot - MAX_SCRIPTS;
        _eventSlots[eventIdx].active = false;
    }
    return true;
}

void ScriptRunner::stopAll() {
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        _slots[i].active = false;
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        _eventSlots[i].active = false;
    }
}

bool ScriptRunner::removeScript(uint8_t id) {
    int slot = findSlotById(id);
    if (slot == -1) {
        return false;
    }

    if (slot < MAX_SCRIPTS) {
        if (_slots[slot].active) _slots[slot].active = false;
        resetScriptState(slot);
        for (uint8_t i = 0; i < _eventHandlerCount; i++) {
            if (_eventHandlers[i].active && _eventHandlers[i].slotId == slot) {
                _eventHandlers[i].active = false;
            }
        }
    } else {
        uint8_t eventIdx = slot - MAX_SCRIPTS;
        if (_eventSlots[eventIdx].active) _eventSlots[eventIdx].active = false;
        resetEventSlot(eventIdx);
    }

    return true;
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

bool ScriptRunner::getNextEventToken(EventSlot& s, char* token, uint16_t& tokenLen) {
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
    if (s.inLoop) {
        if (s.isInfinite) {
            s.pos = s.loopStartPos;
            return;
        }
        if (s.repeatCount > 0) {
            s.repeatCount--;
            if (s.repeatCount > 0) {
                s.pos = s.loopStartPos;
                return;
            }
        }
        s.inLoop = false;
    }
    
    if (!s.isHandler && !s.isPersistent) {
        resetScriptState(idx);
    } else {
        s.active = false;
    }
}

void ScriptRunner::finishEventSlot(EventSlot& s, uint8_t idx) {
    s.active = false;
}

void ScriptRunner::processScript(uint8_t idx, uint32_t now) {
    ScriptState& s = _slots[idx];
    
    if (!s.active || !s.registered) return;
    
    if (s.inWait) {
        if (now >= s.waitUntil) {
            s.inWait = false;
        } else {
            return;
        }
    }
    
    if (now - s.lastExecutionTime < 10) return;
    s.lastExecutionTime = now;
    
    if (s.pos >= s.scriptLen) {
        finishScript(s, idx);
        return;
    }
    
    uint16_t tokenLen = 0;
    if (!getNextToken(s, _tokenBuf, tokenLen)) {
        if (s.pos >= s.scriptLen) {
            finishScript(s, idx);
        }
        return;
    }
    
    processToken(_tokenBuf, s, now);
}

void ScriptRunner::processEventSlot(uint8_t idx, uint32_t now) {
    EventSlot& s = _eventSlots[idx];
    
    if (!s.active || !s.registered) return;
    
    if (s.inWait) {
        if (now >= s.waitUntil) {
            s.inWait = false;
        } else {
            return;
        }
    }
    
    if (now - s.lastExecutionTime < 10) return;
    s.lastExecutionTime = now;
    
    if (s.pos >= s.scriptLen) {
        finishEventSlot(s, idx);
        return;
    }
    
    uint16_t tokenLen = 0;
    if (!getNextEventToken(s, _tokenBuf, tokenLen)) {
        if (s.pos >= s.scriptLen) {
            finishEventSlot(s, idx);
        }
        return;
    }
    
    processToken(_tokenBuf, (ScriptState&)s, now);
}

void ScriptRunner::update() {
    uint32_t now = millis();

    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        processScript(i, now);
    }

    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        processEventSlot(i, now);
    }
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
    if (idx < MAX_ARRAY_VARS && pos < _ctx.arrayLen[idx]) {
        return _ctx.arrayVars[idx][pos];
    }
    return 0;
}

void ScriptRunner::setArrayByte(uint8_t idx, uint8_t pos, uint8_t value) {
    if (idx < MAX_ARRAY_VARS && pos < MAX_ARRAY_SIZE) {
        _ctx.arrayVars[idx][pos] = value;
        if (pos >= _ctx.arrayLen[idx]) {
            _ctx.arrayLen[idx] = pos + 1;
        }
    }
}

uint8_t ScriptRunner::getArrayLen(uint8_t idx) const {
    if (idx < MAX_ARRAY_VARS) return _ctx.arrayLen[idx];
    return 0;
}

bool ScriptRunner::isRunning(uint8_t id) const {
    int slot = findSlotById(id);
    if (slot == -1) return false;
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].active;
    } else {
        uint8_t eventIdx = slot - MAX_SCRIPTS;
        return _eventSlots[eventIdx].active;
    }
}

bool ScriptRunner::isBusy() const {
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        if (_slots[i].active) return true;
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        if (_eventSlots[i].active) return true;
    }
    return false;
}

bool ScriptRunner::isSlotUsed(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].registered;
    } else if (slot < TOTAL_SLOTS) {
        return _eventSlots[slot - MAX_SCRIPTS].registered;
    }
    return false;
}

int ScriptRunner::getSlotId(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].registered ? _slots[slot].id : -1;
    } else if (slot < TOTAL_SLOTS) {
        return _eventSlots[slot - MAX_SCRIPTS].registered ? _eventSlots[slot - MAX_SCRIPTS].id : -1;
    }
    return -1;
}

bool ScriptRunner::isSlotActive(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].active;
    } else if (slot < TOTAL_SLOTS) {
        return _eventSlots[slot - MAX_SCRIPTS].active;
    }
    return false;
}

bool ScriptRunner::isSlotHandler(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].isHandler;
    } else if (slot < TOTAL_SLOTS) {
        return _eventSlots[slot - MAX_SCRIPTS].isHandler;
    }
    return false;
}

uint16_t ScriptRunner::getSlotLen(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].scriptLen;
    } else if (slot < TOTAL_SLOTS) {
        return _eventSlots[slot - MAX_SCRIPTS].scriptLen;
    }
    return 0;
}

uint16_t ScriptRunner::getSlotSize(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return _slots[slot].slotSize;
    } else if (slot < TOTAL_SLOTS) {
        return _eventSlots[slot - MAX_SCRIPTS].slotSize;
    }
    return 0;
}

SlotType ScriptRunner::getSlotType(uint8_t slot) const {
    if (slot < MAX_SCRIPTS) {
        return SLOT_SCRIPT;
    } else if (slot < TOTAL_SLOTS) {
        return SLOT_EVENT;
    }
    return SLOT_SCRIPT;
}

uint8_t ScriptRunner::getTotalSlots() const {
    return TOTAL_SLOTS;
}

uint8_t ScriptRunner::getUsedSlotsCount() const {
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        if (_slots[i].registered) count++;
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        if (_eventSlots[i].registered) count++;
    }
    return count;
}

uint8_t ScriptRunner::getFreeSlotsCount() const {
    return TOTAL_SLOTS - getUsedSlotsCount();
}

uint32_t ScriptRunner::getTotalMemory() const {
    uint32_t total = 0;
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        total += _slots[i].slotSize;
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        total += _eventSlots[i].slotSize;
    }
    return total;
}

uint32_t ScriptRunner::getUsedMemory() const {
    uint32_t used = 0;
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
        if (_slots[i].registered) used += _slots[i].slotSize;
    }
    for (uint8_t i = 0; i < MAX_EVENT_SLOTS; i++) {
        if (_eventSlots[i].registered) used += _eventSlots[i].slotSize;
    }
    return used;
}

uint32_t ScriptRunner::getFreeMemory() const {
    return getTotalMemory() - getUsedMemory();
}

void ScriptRunner::getSlotInfo(uint8_t slot, uint8_t& id, uint16_t& size, uint16_t& used, bool& active, bool& isHandler, SlotType& type) const {
    if (slot >= TOTAL_SLOTS) {
        id = 0;
        size = 0;
        used = 0;
        active = false;
        isHandler = false;
        type = SLOT_SCRIPT;
        return;
    }
    if (slot < MAX_SCRIPTS) {
        id = _slots[slot].id;
        size = _slots[slot].slotSize;
        used = _slots[slot].scriptLen;
        active = _slots[slot].active;
        isHandler = _slots[slot].isHandler;
        type = SLOT_SCRIPT;
    } else {
        uint8_t idx = slot - MAX_SCRIPTS;
        id = _eventSlots[idx].id;
        size = _eventSlots[idx].slotSize;
        used = _eventSlots[idx].scriptLen;
        active = _eventSlots[idx].active;
        isHandler = _eventSlots[idx].isHandler;
        type = SLOT_EVENT;
    }
}

void ScriptRunner::setDataProvider(DataProvider provider) {
    _dataProvider = provider;
    #ifdef ENABLE_PROVIDER_LOGGING
    logProviderSet("DataProvider", provider != nullptr);
    #endif
}

void ScriptRunner::setLogProvider(LogProvider provider) {
    _logProvider = provider;
    #ifdef ENABLE_PROVIDER_LOGGING
    // Не логируем установку логгера через сам логгер
    #endif
}

void ScriptRunner::setPortProvider(PortProvider provider) {
    _portProvider = provider;
    #ifdef ENABLE_PROVIDER_LOGGING
    logProviderSet("PortProvider", provider != nullptr);
    #endif
}

void ScriptRunner::setStateChangeProvider(StateChangeProvider provider) {
    _stateChangeProvider = provider;
    #ifdef ENABLE_PROVIDER_LOGGING
    logProviderSet("StateChangeProvider", provider != nullptr);
    #endif
}

void ScriptRunner::setLoadProvider(LoadProvider provider) {
    #ifdef ENABLE_LOAD_CACHE
    _originalLoadProvider = provider;
    if (provider) {
        _loadProvider = cachedLoadProviderWrapper;
        #ifdef ENABLE_PROVIDER_LOGGING
        if (_logProvider) _logProvider("[R] LoadProvider set to CACHED wrapper");
        #endif
    } else {
        _loadProvider = nullptr;
        #ifdef ENABLE_PROVIDER_LOGGING
        if (_logProvider) _logProvider("[R] LoadProvider set to NULL");
        #endif
    }
    #else
    _loadProvider = provider;
    #ifdef ENABLE_PROVIDER_LOGGING
    if (_logProvider) _logProvider("[R] LoadProvider set to ORIGINAL (no cache)");
    #endif
    #endif
}

void ScriptRunner::setOutput(uint8_t gpio, uint16_t value) {
    if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;
    
    #ifdef ENABLE_PROVIDER_LOGGING
    logPortAction(gpio, PORT_WRITE, value);
    #endif
    
    if (_portProvider) {
        _portProvider(gpio, PORT_WRITE, value);
        if (_stateChangeProvider) {
            _stateChangeProvider(gpio, 0, value);
        }
    }
}

void ScriptRunner::setError(const char* msg) {
    if (_logProvider) {
        snprintf(_logBuf, sizeof(_logBuf), "[R] ERR: %s", msg);
        _logProvider(_logBuf);
    }
}

#ifdef ENABLE_PROVIDER_LOGGING

void ScriptRunner::logProviderSet(const char* name, bool enabled) {
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[R] %s: %s", name, enabled ? "SET" : "CLEARED");
        _logProvider(buf);
    }
}

void ScriptRunner::logPortAction(uint8_t gpio, PortAction action, uint16_t value) {
    if (_logProvider) {
        char buf[64];
        const char* actionStr = (action == PORT_READ) ? "READ" : "WRITE";
        snprintf(buf, sizeof(buf), "[R] portProvider: gpio: %d %s value: %d", gpio, actionStr, value);
        _logProvider(buf);
    }
}

void ScriptRunner::logDataAction(const char* id, DataKind kind, bool write, const char* value) {
    if (_logProvider) {
        char buf[128];
        const char* kindStr = "";
        switch (kind) {
            case KIND_UINT: kindStr = "UINT"; break;
            case KIND_INT: kindStr = "INT"; break;
            case KIND_FLOAT: kindStr = "FLOAT"; break;
            case KIND_STRING: kindStr = "STRING"; break;
        }
        const char* actionStr = write ? "WRITE" : "READ";
        snprintf(buf, sizeof(buf), "[R] dataProvider: %s %s %s: %s", id, kindStr, actionStr, value ? value : "");
        _logProvider(buf);
    }
}

void ScriptRunner::logLoadAction(uint8_t id, uint16_t len, bool cached) {
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[R] loadProvider: id: %d len: %d %s", 
                 id, len, cached ? "(CACHED)" : "(LOADED)");
        _logProvider(buf);
    }
}

#endif

#ifdef ENABLE_LOAD_CACHE

int ScriptRunner::findInLoadCache(uint8_t id, char* buffer, uint16_t& len) {
    for (uint8_t i = 0; i < LOAD_CACHE_SIZE; i++) {
        if (_loadCache[i].valid && _loadCache[i].id == id) {
            _loadCache[i].lastAccess = millis();
            _loadCache[i].accessCount++;
            strcpy(buffer, _loadCache[i].script);
            len = _loadCache[i].len;
            _loadCacheHits++;
            return i;
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
    
    int slot = findEmptyLoadSlot();
    if (slot == -1) {
        slot = findLeastUsedSlot();
    }
    
    if (slot != -1) {
        _loadCache[slot].id = id;
        strcpy(_loadCache[slot].script, script);
        _loadCache[slot].len = len;
        _loadCache[slot].valid = true;
        _loadCache[slot].lastAccess = millis();
        _loadCache[slot].accessCount = 1;
    }
}

int ScriptRunner::findEmptyLoadSlot() const {
    for (uint8_t i = 0; i < LOAD_CACHE_SIZE; i++) {
        if (!_loadCache[i].valid) return i;
    }
    return -1;
}

int ScriptRunner::findLeastUsedSlot() const {
    int least = 0;
    for (uint8_t i = 1; i < LOAD_CACHE_SIZE; i++) {
        if (_loadCache[i].accessCount < _loadCache[least].accessCount) {
            least = i;
        }
    }
    return least;
}

bool ScriptRunner::cachedLoadProviderWrapper(uint8_t id, char* buffer, uint16_t& len) {
    if (!_instance) return false;

    int found = _instance->findInLoadCache(id, buffer, len);
    if (found != -1) {
        #ifdef ENABLE_PROVIDER_LOGGING
        _instance->logLoadAction(id, len, true);
        #endif
        return true;
    }
    
    if (_instance->_originalLoadProvider) {
        bool result = _instance->_originalLoadProvider(id, buffer, len);
        if (result && len > 0) {
            #ifdef ENABLE_PROVIDER_LOGGING
            _instance->logLoadAction(id, len, false);
            #endif
            _instance->addToLoadCache(id, buffer, len);
        }
        return result;
    }
    
    return false;
}

#endif // ENABLE_LOAD_CACHE

ScriptRunner::ScriptRunner()
    : _dataProvider(nullptr), _logProvider(nullptr), _portProvider(nullptr),
      _stateChangeProvider(nullptr), _loadProvider(nullptr), _eventHandlerCount(0) {
    _instance = this;

    initSlotPools();

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
        for (uint8_t j = 0; j < MAX_ARRAY_SIZE; j++) {
            _ctx.arrayVars[i][j] = 0;
        }
    }

    for (uint8_t i = 0; i < MAX_EVENT_HANDLERS; i++) {
        _eventHandlers[i].active = false;
        _eventHandlers[i].hash = 0;
        _eventHandlers[i].slotId = 0;
    }
}

ScriptRunner::~ScriptRunner() {
    _instance = nullptr;
}

ScriptRunner scriptRunner;