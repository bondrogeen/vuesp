#ifdef ARDUINO
#include "./runner.h"
#else
#include "runner.h"
#endif

ScriptRunner* ScriptRunner::_instance = nullptr;

ScriptRunner::ScriptRunner(ScriptConflict defaultStrategy)
    : _defaultStrategy(defaultStrategy)
    , _dataProvider(nullptr)
    , _logProvider(nullptr)
    , _portProvider(nullptr)
    , _stateChangeProvider(nullptr)
    , _lastStateChangeTime(0)
    , _activeCount(0)
    , _registryCount(0)
    , _queueHead(0)
    , _queueTail(0)
    , _queueCount(0) {
    
    _instance = this;

    if (_logProvider) _logProvider("[Runner] Constructor called");

    for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) resetScriptState(i);

    for (int i = 0; i < MAX_UINT_VARS; i++) _ctx.uintVars[i] = 0;
    for (int i = 0; i < MAX_INT_VARS; i++) _ctx.intVars[i] = 0;
    for (int i = 0; i < MAX_FLOAT_VARS; i++) _ctx.floatVars[i] = 0.0;
    for (int i = 0; i < MAX_STRING_VARS; i++) _ctx.stringVars[i][0] = '\0';

    for (int i = 0; i < MAX_ARRAY_VARS; i++) {
        _ctx.arrayLen[i] = 0;
        for (int j = 0; j < MAX_ARRAY_SIZE; j++) {
            _ctx.arrayVars[i][j] = 0;
        }
    }

    if (_logProvider) _logProvider("[Runner] Constructor finished");
}

ScriptRunner::~ScriptRunner() {
    if (_logProvider) _logProvider("[Runner] Destructor called");
    _instance = nullptr;
}

void ScriptRunner::setDataProvider(DataProvider provider) { 
    _dataProvider = provider; 
    if (_logProvider) _logProvider("[Runner] DataProvider set");
}

void ScriptRunner::setLogProvider(LogProvider provider) { 
    _logProvider = provider; 
    if (_logProvider) _logProvider("[Runner] LogProvider set");
}

void ScriptRunner::setPortProvider(PortProvider provider) { 
    _portProvider = provider; 
    if (_logProvider) _logProvider("[Runner] PortProvider set");
}

void ScriptRunner::setStateChangeProvider(StateChangeProvider provider) { 
    _stateChangeProvider = provider; 
    if (_logProvider) _logProvider("[Runner] StateChangeProvider set");
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
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] setUintVar[%d]=%u", idx, value);
        _logProvider(buf);
    }
}

void ScriptRunner::setIntVar(uint8_t idx, int32_t value) {
    if (idx < MAX_INT_VARS) _ctx.intVars[idx] = value;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] setIntVar[%d]=%d", idx, value);
        _logProvider(buf);
    }
}

void ScriptRunner::setFloatVar(uint8_t idx, float value) {
    if (idx < MAX_FLOAT_VARS) _ctx.floatVars[idx] = (double)value;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] setFloatVar[%d]=%.2f", idx, value);
        _logProvider(buf);
    }
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

void ScriptRunner::resetScriptState(int idx) {
    ScriptState& s = _active[idx];
    s.active = false;
    s.id = 0;
    s.script = nullptr;
    s.pos = 0;
    s.scriptLen = 0;
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
    s.waitEvent = nullptr;
    s.waitEventHash = 0;
    s.inEvent = false;
    s.eventHash = 0;
    s.eventStartTime = 0;
    s.eventTimeout = 0;
    s.tempResult = 0;
    s.hasTempResult = false;
    s.isWhile = false;
    s.whileCondition = nullptr;
    s.whileConditionBuffer[0] = '\0';
}

void ScriptRunner::addToActiveList(uint8_t idx) {
    for (int i = 0; i < _activeCount; i++) {
        if (_activeList[i] == idx) return;
    }
    if (_activeCount < MAX_ACTIVE_SCRIPTS) {
        _activeList[_activeCount++] = idx;
        if (_logProvider) {
            char buf[64];
            snprintf(buf, 64, "[Runner] Script %d activated, activeCount=%d", _active[idx].id, _activeCount);
            _logProvider(buf);
        }
    }
}

void ScriptRunner::removeFromActiveList(uint8_t idx) {
    for (int i = 0; i < _activeCount; i++) {
        if (_activeList[i] == idx) {
            if (_logProvider) {
                char buf[64];
                snprintf(buf, 64, "[Runner] Script %d removed, activeCount=%d", _active[idx].id, _activeCount - 1);
                _logProvider(buf);
            }
            _activeList[i] = _activeList[--_activeCount];
            return;
        }
    }
}

int ScriptRunner::findById(uint8_t id) const {
    for (int i = 0; i < _activeCount; i++) {
        uint8_t idx = _activeList[i];
        if (_active[idx].active && _active[idx].id == id) {
            return idx;
        }
    }
    return -1;
}

int ScriptRunner::findInRegistry(uint8_t id) const {
    for (int i = 0; i < _registryCount; i++) {
        if (_registry[i].id == id) return i;
    }
    return -1;
}

void ScriptRunner::addToRegistry(uint8_t id, const char* script) {
    int existing = findInRegistry(id);
    if (existing != -1) {
        _registry[existing].script = script;
        if (_logProvider) {
            char buf[64];
            snprintf(buf, 64, "[Runner] Script %d updated in registry", id);
            _logProvider(buf);
        }
        return;
    }
    if (_registryCount < MAX_REGISTERED) {
        _registry[_registryCount].id = id;
        _registry[_registryCount].script = script;
        _registryCount++;
        if (_logProvider) {
            char buf[64];
            snprintf(buf, 64, "[Runner] Script %d registered (total: %d)", id, _registryCount);
            _logProvider(buf);
        }
    }
}

bool ScriptRunner::addToQueue(uint8_t id, const char* script, uint16_t len) {
    if (_queueCount >= MAX_QUEUE_SIZE) return false;
    _queueId[_queueTail] = id;
    _queueScript[_queueTail] = script;
    _queueLen[_queueTail] = len;
    _queueTail = (_queueTail + 1) % MAX_QUEUE_SIZE;
    _queueCount++;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] Script %d added to queue (queueCount=%d)", id, _queueCount);
        _logProvider(buf);
    }
    return true;
}

void ScriptRunner::activateSlot(int idx, uint8_t id, const char* script, uint16_t len) {
    resetScriptState(idx);
    _active[idx].active = true;
    _active[idx].id = id;
    _active[idx].script = script;
    _active[idx].scriptLen = len;
    _active[idx].startTime = millis();
    addToActiveList(idx);
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] Script %d activated in slot %d", id, idx);
        _logProvider(buf);
    }
}

bool ScriptRunner::registerScript(uint8_t id, const char* script) {
    uint16_t len = strlen(script);
    if (len > MAX_SCRIPT_LEN) {
        if (_logProvider) _logProvider("[Runner] ERROR: Script too long!");
        return false;
    }
    addToRegistry(id, script);
    if (_logProvider) _logProvider("[Runner] registerScript OK");
    return true;
}

bool ScriptRunner::addScript(uint8_t id, const char* script, ScriptConflict strategy) {
    uint16_t len = strlen(script);
    if (len > MAX_SCRIPT_LEN) {
        if (_logProvider) _logProvider("[Runner] ERROR: Script too long!");
        return false;
    }

    addToRegistry(id, script);

    int existingIdx = findById(id);
    if (existingIdx != -1) {
        switch (strategy) {
            case IGNORE:
                if (_logProvider) _logProvider("[Runner] Script already running, IGNORE");
                return true;
            case RESTART:
                if (_logProvider) _logProvider("[Runner] Script already running, RESTART");
                activateSlot(existingIdx, id, script, len);
                return true;
            case RESTART_IF_SAME:
                if (strcmp(_active[existingIdx].script, script) == 0) {
                    if (_logProvider) _logProvider("[Runner] Script already running, RESTART_IF_SAME");
                    activateSlot(existingIdx, id, script, len);
                    return true;
                }
                break;
            case ADD_QUEUE:
                if (_logProvider) _logProvider("[Runner] Script already running, ADD_QUEUE");
                return addToQueue(id, script, len);
        }
    }

    for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
        if (!_active[i].active) {
            activateSlot(i, id, script, len);
            return true;
        }
    }

    if (_logProvider) _logProvider("[Runner] No free slots, adding to queue");
    return addToQueue(id, script, len);
}

bool ScriptRunner::runScript(uint8_t id) {
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] runScript called for ID %d", id);
        _logProvider(buf);
    }

    int regIdx = findInRegistry(id);
    if (regIdx == -1) {
        if (_logProvider) _logProvider("[Runner] ERROR: Script not found in registry!");
        return false;
    }
    const char* script = _registry[regIdx].script;
    uint16_t len = strlen(script);
    int existingIdx = findById(id);
    if (existingIdx != -1) {
        activateSlot(existingIdx, id, script, len);
        return true;
    }
    for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
        if (!_active[i].active) {
            activateSlot(i, id, script, len);
            return true;
        }
    }
    if (_logProvider) _logProvider("[Runner] No free slots, adding to queue");
    return addToQueue(id, script, len);
}

bool ScriptRunner::stopScript(uint8_t id) {
    int idx = findById(id);
    if (idx == -1) return false;
    _active[idx].active = false;
    removeFromActiveList(idx);
    return true;
}

void ScriptRunner::stopAll() {
    for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
        _active[i].active = false;
    }
    _activeCount = 0;
    _queueCount = 0;
    _queueHead = 0;
    _queueTail = 0;
    if (_logProvider) _logProvider("[Runner] All scripts stopped");
}

bool ScriptRunner::isRunning(uint8_t id) const { return findById(id) != -1; }
bool ScriptRunner::isBusy() const { return _activeCount > 0 || _queueCount > 0; }

static bool isDigit(char c) { return c >= '0' && c <= '9'; }
static bool isAlpha(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'); }
static bool isAlphaNum(char c) { return isAlpha(c) || isDigit(c) || c == '_'; }

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
    int len = *p - start;
    if (len >= MAX_STRING_LEN) len = MAX_STRING_LEN - 1;
    strncpy(buf, start, len);
    buf[len] = '\0';
    if (**p == '\'') (*p)++;
    return true;
}

uint32_t ScriptRunner::parseTime(const char* str) {
    const char* p = str;
    while (isDigit(*p) || *p == '.') p++;
    char numStr[16];
    strncpy(numStr, str, p - str);
    numStr[p - str] = '\0';
    double value = atof(numStr);
    char unit = *p;
    switch (unit) {
        case 'u':
            return (uint32_t)value;
        case 's':
            return (uint32_t)(value * 1000);
        case 'm':
            return (uint32_t)(value * 60000);
        case 'h':
            return (uint32_t)(value * 3600000);
        default:
            return (uint32_t)(value * 1000);
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
        int len = p - start;
        if (len > 31) len = 31;
        strncpy(result.values[result.count], start, len);
        result.values[result.count][len] = '\0';
        result.count++;
        if (p < close && *p == ',') p++;
    }

    return result;
}

bool ScriptRunner::parseValue(const char** p, ScriptState& s, int32_t& result) {
    const char* pos = *p;

    if (*pos == '$') {
        char type = pos[1];
        pos += 2;
        int index = parseUint(&pos);

        switch (type) {
            case 'v':
                if (index < MAX_UINT_VARS) {
                    result = (int32_t)_ctx.uintVars[index];
                    *p = pos;
                    return true;
                }
                break;
            case 'i':
                if (index < MAX_INT_VARS) {
                    result = _ctx.intVars[index];
                    *p = pos;
                    return true;
                }
                break;
            case 'f':
                if (index < MAX_FLOAT_VARS) {
                    result = (int32_t)_ctx.floatVars[index];
                    *p = pos;
                    return true;
                }
                break;
            case 's':
                result = 0;
                *p = pos;
                return true;
            case 'p': {
                uint16_t val = 0;
                if (_portProvider && _portProvider(index, PORT_READ, val)) {
                    result = (int32_t)val;
                    *p = pos;
                    return true;
                }
                return false;
            }
            default: {
                if (_dataProvider) {
                    const char* start = pos - 2;
                    const char* end = start + 1;
                    while (isAlphaNum(*end)) end++;
                    int len = end - start;
                    if (len > 31) len = 31;
                    char name[32];
                    strncpy(name, start, len);
                    name[len] = '\0';
                    DataValue dv;
                    if (_dataProvider(name, KIND_INT, dv, false)) {
                        result = (int32_t)dv.intVal;
                        *p = end;
                        return true;
                    }
                    if (_dataProvider(name, KIND_UINT, dv, false)) {
                        result = (int32_t)dv.uintVal;
                        *p = end;
                        return true;
                    }
                    if (_dataProvider(name, KIND_FLOAT, dv, false)) {
                        result = (int32_t)dv.floatVal;
                        *p = end;
                        return true;
                    }
                }
                return false;
            }
        }
        return false;
    }

    if (*pos == '\'') {
        result = 0;
        char buf[MAX_STRING_LEN];
        if (parseString(&pos, buf)) {
            *p = pos;
            return true;
        }
        return false;
    }

    if (isDigit(*pos) || *pos == '.' || *pos == '-') {
        char* end;
        double val = strtod(pos, &end);
        if (end > pos) {
            result = (int32_t)val;
            *p = end;
            return true;
        }
        return false;
    }

    return false;
}

bool ScriptRunner::parseArray(const char** p, uint8_t idx) {
    const char* pos = *p;

    if (*pos != '{') return false;
    pos++;

    int i = 0;
    while (*pos && *pos != '}' && i < MAX_ARRAY_SIZE) {
        int val = parseInt(&pos);
        _ctx.arrayVars[idx][i] = (uint8_t)val;
        i++;
        if (*pos == ';') pos++;
    }

    if (*pos != '}') return false;
    pos++;

    _ctx.arrayLen[idx] = i;
    *p = pos;
    return true;
}

uint32_t ScriptRunner::hashEvent(const char* str) const {
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

bool ScriptRunner::handleCall(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    uint8_t id = atoi(params.values[0]);
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleCall: %d", id);
        _logProvider(buf);
    }
    int regIdx = findInRegistry(id);
    if (regIdx != -1) {
        addScript(id, _registry[regIdx].script, RESTART);
        return true;
    }
    return false;
}

bool ScriptRunner::handleOn(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    
    char eventName[64] = {0};
    for (int i = 0; i < params.count && i < 8; i++) {
        if (i > 0) strcat(eventName, ",");
        strcat(eventName, params.values[i]);
    }
    
    s.eventHash = hashEvent(eventName);
    s.inEvent = true;
    s.eventStartTime = millis();
    if (params.count > 1) {
        s.eventTimeout = parseTime(params.values[1]);
    } else {
        s.eventTimeout = 0;
    }
    if (_logProvider) {
        char buf[128];
        snprintf(buf, 128, "[Runner] handleOn: event=%s", eventName);
        _logProvider(buf);
    }
    return true;
}

bool ScriptRunner::handleWait(const Params& params, ScriptState& s, uint32_t now) {
    if (params.count < 1) return false;
    uint32_t duration = parseTime(params.values[0]);
    if (duration > 0) {
        s.inWait = true;
        s.waitUntil = now + duration;
    }
    if (params.count > 1) {
        s.waitEvent = params.values[1];
        s.waitEventHash = hashEvent(s.waitEvent);
    }
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleWait: duration=%u", duration);
        _logProvider(buf);
    }
    return true;
}

bool ScriptRunner::handleWhile(const char* params, ScriptState& s) {
    if (s.inLoop) {
        setError("Nested loops are not allowed");
        return false;
    }
    
    strncpy(s.whileConditionBuffer, params, 63);
    s.whileConditionBuffer[63] = '\0';
    s.whileCondition = s.whileConditionBuffer;
    s.isWhile = true;
    
    bool condition = parseCondition(s.whileCondition, s);
    
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] while: condition=%d (%s)", condition, s.whileCondition);
        _logProvider(buf);
    }
    
    if (!condition) {
        return true;
    }
    
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
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleIf: result=%d", s.ifResult);
        _logProvider(buf);
    }
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
        if (s.isWhile && s.whileCondition) {
            bool condition = parseCondition(s.whileCondition, s);
            if (condition) {
                s.pos = s.loopStartPos;
                return true;
            }
            s.inLoop = false;
            s.isWhile = false;
            s.whileCondition = nullptr;
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

    return false;
}

bool ScriptRunner::parseCondition(const char* token, ScriptState& s) {
    const char* p = token;
    bool result = false;
    bool hasResult = false;
    char pendingOp = '\0';
    
    while (*p && *p != ',') {
        int32_t leftVal;
        if (!parseValue(&p, s, leftVal)) {
            s.ifResult = false;
            return false;
        }
        
        char op1 = *p;
        char op2 = '\0';
        if (p[1] == '=') {
            op2 = p[1];
            p += 2;
        } else {
            p++;
        }
        
        int32_t rightVal;
        if (!parseValue(&p, s, rightVal)) {
            s.ifResult = false;
            return false;
        }
        
        bool condResult = false;
        if (op1 == '=' && op2 == '=') condResult = (leftVal == rightVal);
        else if (op1 == '!' && op2 == '=') condResult = (leftVal != rightVal);
        else if (op1 == '>' && op2 == '\0') condResult = (leftVal > rightVal);
        else if (op1 == '<' && op2 == '\0') condResult = (leftVal < rightVal);
        else if (op1 == '>' && op2 == '=') condResult = (leftVal >= rightVal);
        else if (op1 == '<' && op2 == '=') condResult = (leftVal <= rightVal);
        
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
        
        if (*p == '&' && p[1] == '&') {
            pendingOp = '&';
            p += 2;
        } else if (*p == '|' && p[1] == '|') {
            pendingOp = '|';
            p += 2;
        } else {
            break;
        }
    }
    
    s.ifResult = result;
    return result;
}

bool ScriptRunner::handleSet(const Params& params, ScriptState& s) {
    if (params.count < 3) return false;

    const char* name = params.values[0];
    if (name[0] != '$') return false;

    char type = name[1];
    int idx = name[2] - '0';
    int pos = atoi(params.values[1]);
    int val = atoi(params.values[2]);

    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleSet: %s[%d]=%d", name, pos, val);
        _logProvider(buf);
    }

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
    int idx = name[2] - '0';
    int pos = atoi(params.values[1]);

    uint8_t val = 0;

    if (type == 'a' && idx < MAX_ARRAY_VARS) {
        if (pos >= 0 && pos < _ctx.arrayLen[idx]) {
            val = _ctx.arrayVars[idx][pos];
        }
    } else if (type == 's' && idx < MAX_STRING_VARS) {
        int len = strlen(_ctx.stringVars[idx]);
        if (pos >= 0 && pos < len) {
            val = (uint8_t)_ctx.stringVars[idx][pos];
        }
    } else {
        return false;
    }

    s.tempResult = val;
    s.hasTempResult = true;

    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleGet: %s[%d]=%d", name, pos, val);
        _logProvider(buf);
    }

    return true;
}

bool ScriptRunner::handleLen(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;

    const char* name = params.values[0];
    if (name[0] != '$') return false;

    char type = name[1];
    int idx = name[2] - '0';

    int len = 0;

    switch (type) {
        case 'a':
            if (idx < MAX_ARRAY_VARS) {
                len = _ctx.arrayLen[idx];
            }
            break;
        case 's':
            if (idx < MAX_STRING_VARS) {
                len = strlen(_ctx.stringVars[idx]);
            }
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

    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleLen: %s = %d", name, len);
        _logProvider(buf);
    }

    return true;
}

bool ScriptRunner::handleChr(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    int val = atoi(params.values[0]);
    s.tempResult = val;
    s.hasTempResult = true;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleChr: %d -> '%c'", val, (char)val);
        _logProvider(buf);
    }
    return true;
}

bool ScriptRunner::handleOrd(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    const char* arg = params.values[0];
    uint8_t val = 0;
    
    if (arg[0] == '\'') {
        const char* p = arg;
        char buf[2];
        if (parseString(&p, buf)) {
            val = (uint8_t)buf[0];
        }
    } else if (arg[0] == '$' && arg[1] == 's') {
        int idx = arg[2] - '0';
        if (idx < MAX_STRING_VARS) {
            val = (uint8_t)_ctx.stringVars[idx][0];
        }
    } else if (arg[0] == '$' && arg[1] == 'v') {
        int idx = arg[2] - '0';
        if (idx < MAX_UINT_VARS) {
            val = (uint8_t)_ctx.uintVars[idx];
        }
    } else if (arg[0] == '$' && arg[1] == 'i') {
        int idx = arg[2] - '0';
        if (idx < MAX_INT_VARS) {
            val = (uint8_t)_ctx.intVars[idx];
        }
    } else if (arg[0] == '$' && arg[1] == 'f') {
        int idx = arg[2] - '0';
        if (idx < MAX_FLOAT_VARS) {
            val = (uint8_t)_ctx.floatVars[idx];
        }
    }
    
    s.tempResult = val;
    s.hasTempResult = true;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] handleOrd: %s = %d", arg, val);
        _logProvider(buf);
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
    int index = 0;
    const char* p = left + 2;
    
    if ((type == 'v' || type == 'i' || type == 'f' || type == 's' || type == 'a' || type == 'p') && isDigit(*p)) {
        isInternal = true;
        while (isDigit(*p)) {
            index = index * 10 + (*p - '0');
            p++;
        }
    }
    
    if (_logProvider) {
        char buf[128];
        snprintf(buf, 128, "[Runner] handleAssignment: %s = %s (internal=%d, type=%c, index=%d)", left, right, isInternal, type, index);
        _logProvider(buf);
    }
    
    if (isInternal) {
        if (type == 'p') {
            if (isDigit(*right) || *right == '-') {
                const char* p2 = right;
                int32_t val = parseInt(&p2);
                setOutput(index, (uint16_t)val);
                if (_logProvider) {
                    char buf[64];
                    snprintf(buf, 64, "[Runner] Port set: gpio=%d, value=%d", index, val);
                    _logProvider(buf);
                }
                return true;
            }
            else {
                uint16_t val = 0;
                if (_portProvider && _portProvider(index, PORT_READ, val)) {
                    s.tempResult = val;
                    s.hasTempResult = true;
                    if (_logProvider) {
                        char buf[64];
                        snprintf(buf, 64, "[Runner] Port read: gpio=%d, value=%d", index, val);
                        _logProvider(buf);
                    }
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
                int srcIdx = parseUint(&right);
                if (srcIdx < MAX_ARRAY_VARS) {
                    _ctx.arrayLen[index] = _ctx.arrayLen[srcIdx];
                    for (int i = 0; i < _ctx.arrayLen[index]; i++) {
                        _ctx.arrayVars[index][i] = _ctx.arrayVars[srcIdx][i];
                    }
                    return true;
                }
            }
            if (*right == '$' && right[1] == 's') {
                right += 2;
                int srcIdx = parseUint(&right);
                if (srcIdx < MAX_STRING_VARS) {
                    int len = strlen(_ctx.stringVars[srcIdx]);
                    if (len > MAX_ARRAY_SIZE) len = MAX_ARRAY_SIZE;
                    for (int i = 0; i < len; i++) {
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
                char buf[MAX_STRING_LEN];
                if (parseString(&right, buf)) {
                    strcpy(_ctx.stringVars[index], buf);
                    if (_logProvider) {
                        char logBuf[128];
                        snprintf(logBuf, 128, "[Runner] String assigned: %s", buf);
                        _logProvider(logBuf);
                    }
                    return true;
                }
            }
            else if (strchr(right, '+') != nullptr) {
                const char* p2 = right;
                char leftStr[MAX_STRING_LEN] = {0};
                char rightStr[MAX_STRING_LEN] = {0};

                if (*p2 == '$' && p2[1] == 's') {
                    p2 += 2;
                    int leftIdx = parseUint(&p2);
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
                    int rightIdx = parseUint(&p2);
                    if (rightIdx < MAX_UINT_VARS) {
                        snprintf(rightStr, MAX_STRING_LEN, "%u", _ctx.uintVars[rightIdx]);
                    }
                } else if (*p2 == '$' && p2[1] == 'i') {
                    p2 += 2;
                    int rightIdx = parseUint(&p2);
                    if (rightIdx < MAX_INT_VARS) {
                        snprintf(rightStr, MAX_STRING_LEN, "%d", _ctx.intVars[rightIdx]);
                    }
                } else if (*p2 == '$' && p2[1] == 'f') {
                    p2 += 2;
                    int rightIdx = parseUint(&p2);
                    if (rightIdx < MAX_FLOAT_VARS) {
                        snprintf(rightStr, MAX_STRING_LEN, "%.2f", _ctx.floatVars[rightIdx]);
                    }
                } else if (*p2 == '$' && p2[1] == 's') {
                    p2 += 2;
                    int rightIdx = parseUint(&p2);
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
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] String concat result: %s", result);
                    _logProvider(logBuf);
                }
                return true;
            }
            else if (*right == '$' && right[1] == 's') {
                right += 2;
                int srcIdx = parseUint(&right);
                if (srcIdx < MAX_STRING_VARS) {
                    strcpy(_ctx.stringVars[index], _ctx.stringVars[srcIdx]);
                    return true;
                }
            }
            else if (s.hasTempResult) {
                char buf[MAX_STRING_LEN];
                snprintf(buf, MAX_STRING_LEN, "%d", s.tempResult);
                strcpy(_ctx.stringVars[index], buf);
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
            }
            else if (*p2 == '$') {
                if (p2[1] == 'f') {
                    const char* temp = p2 + 2;
                    int idx = parseUint(&temp);
                    if (idx < MAX_FLOAT_VARS) {
                        leftFloatVal = (float)_ctx.floatVars[idx];
                        leftIsFloat = true;
                        hasLeft = true;
                        p2 = temp;
                    }
                }
                else if (p2[1] == 'p') {
                    const char* temp = p2 + 2;
                    int gpio = parseUint(&temp);
                    uint16_t val = 0;
                    if (_portProvider && _portProvider(gpio, PORT_READ, val)) {
                        leftVal = (int32_t)val;
                        hasLeft = true;
                        p2 = temp;
                        if (_logProvider) {
                            char buf[64];
                            snprintf(buf, 64, "[Runner] Port read: gpio=%d, value=%d", gpio, val);
                            _logProvider(buf);
                        }
                    } else {
                        if (_logProvider) {
                            char buf[64];
                            snprintf(buf, 64, "[Runner] ERROR: Failed to read port %d", gpio);
                            _logProvider(buf);
                        }
                        return false;
                    }
                } else {
                    if (parseValue(&p2, s, leftVal)) {
                        hasLeft = true;
                    }
                }
            }
            else if (isDigit(*p2) || *p2 == '.' || *p2 == '-') {
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
                        int idx = parseUint(&temp);
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
                        case '~': resultInt = ~leftVal; break;
                        case '<': resultInt = leftVal << (rightVal & 31); break;
                        case '>': resultInt = leftVal >> (rightVal & 31); break;
                        default: return false;
                    }
                }

                if (_logProvider) {
                    char logBuf[128];
                    if (resultIsFloat) {
                        snprintf(logBuf, 128, "[Runner] Operation: %f %c %f = %f", 
                            leftIsFloat ? leftFloatVal : (float)leftVal, op1, 
                            rightIsFloat ? rightFloatVal : (float)rightVal, resultFloat);
                    } else {
                        snprintf(logBuf, 128, "[Runner] Operation: %d %c %d = %d", leftVal, op1, rightVal, resultInt);
                    }
                    _logProvider(logBuf);
                }

                if (type == 'v') {
                    _ctx.uintVars[index] = resultIsFloat ? (uint32_t)resultFloat : (uint32_t)resultInt;
                } else if (type == 'i') {
                    _ctx.intVars[index] = resultIsFloat ? (int32_t)resultFloat : resultInt;
                } else if (type == 'f') {
                    _ctx.floatVars[index] = resultIsFloat ? (double)resultFloat : (double)resultInt;
                }
                
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] Saved $%c%d = %d", type, index, resultIsFloat ? (int)resultFloat : resultInt);
                    _logProvider(logBuf);
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
                
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] Saved $%c%d = %d", type, index, leftIsFloat ? (int)leftFloatVal : leftVal);
                    _logProvider(logBuf);
                }
                
                return true;
            }
        }
        
        return false;
    }

    if (_dataProvider) {
        char name[32];
        const char* leftStart = left;
        const char* leftEnd = left;
        while (*leftEnd && *leftEnd != '=' && *leftEnd != ',' && *leftEnd != ' ') {
            leftEnd++;
        }
        int nameLen = leftEnd - leftStart;
        if (nameLen > 31) nameLen = 31;
        strncpy(name, leftStart, nameLen);
        name[nameLen] = '\0';
        
        if (*right == '\'') {
            const char* p = right;
            char buf[MAX_STRING_LEN];
            if (parseString(&p, buf)) {
                DataValue dv;
                dv.stringVal.data = (uint8_t*)buf;
                dv.stringVal.len = strlen(buf);
                if (_dataProvider(name, KIND_STRING, dv, true)) {
                    if (_logProvider) {
                        char logBuf[128];
                        snprintf(logBuf, 128, "[Runner] External set: %s = %s", name, buf);
                        _logProvider(logBuf);
                    }
                    return true;
                }
            }
            return false;
        }
        
        else if (strchr(right, '+') != nullptr || 
                 strchr(right, '-') != nullptr || 
                 strchr(right, '*') != nullptr || 
                 strchr(right, '/') != nullptr) {
            
            if (_logProvider) {
                char logBuf[128];
                snprintf(logBuf, 128, "[Runner] External expression detected: %s", right);
                _logProvider(logBuf);
            }
            
            const char* p = right;
            int32_t leftVal = 0;
            bool leftParsed = false;
            
            if (*p == '$') {
                const char* nameStart = p + 1;
                const char* nameEnd = nameStart;
                while (*nameEnd && *nameEnd != '+' && *nameEnd != '-' && *nameEnd != '*' && *nameEnd != '/' && *nameEnd != ',' && *nameEnd != ' ') {
                    nameEnd++;
                }
                
                int varNameLen = nameEnd - nameStart;
                char varName[32];
                if (varNameLen > 31) varNameLen = 31;
                strncpy(varName, nameStart, varNameLen);
                varName[varNameLen] = '\0';
                
                if (varNameLen == 2 && isDigit(varName[1])) {
                    char varType = varName[0];
                    int idx = varName[1] - '0';
                    
                    if (varType == 'v' && idx < MAX_UINT_VARS) {
                        leftVal = (int32_t)_ctx.uintVars[idx];
                        leftParsed = true;
                    } else if (varType == 'i' && idx < MAX_INT_VARS) {
                        leftVal = _ctx.intVars[idx];
                        leftParsed = true;
                    } else if (varType == 'f' && idx < MAX_FLOAT_VARS) {
                        leftVal = (int32_t)_ctx.floatVars[idx];
                        leftParsed = true;
                    } else if (varType == 's' && idx < MAX_STRING_VARS) {
                        leftVal = 0;
                        leftParsed = true;
                    }
                }
                
                if (!leftParsed) {
                    if (_dataProvider) {
                        DataValue dv;
                        char fullVarName[33];
                        snprintf(fullVarName, 33, "$%s", varName);
                        
                        if (_dataProvider(fullVarName, KIND_INT, dv, false)) {
                            leftVal = dv.intVal;
                            leftParsed = true;
                        }
                    }
                }
                p = nameEnd;
            } else if (isDigit(*p) || *p == '.') {
                leftVal = parseInt(&p);
                leftParsed = true;
            }
            
            if (!leftParsed) {
                if (_logProvider) _logProvider("[Runner] ERROR: Failed to parse left part");
                return false;
            }
            
            while (*p && (*p != '+' && *p != '-' && *p != '*' && *p != '/')) {
                p++;
            }
            
            if (!*p) {
                if (_logProvider) _logProvider("[Runner] ERROR: Operator not found");
                return false;
            }
            
            char op = *p;
            p++;
            
            int32_t rightVal = 0;
            bool rightParsed = false;
            
            if (*p == '$') {
                const char* rNameStart = p + 1;
                const char* rNameEnd = rNameStart;
                while (*rNameEnd && *rNameEnd != '+' && *rNameEnd != '-' && *rNameEnd != '*' && *rNameEnd != '/' && *rNameEnd != ',' && *rNameEnd != ' ') {
                    rNameEnd++;
                }
                
                int rNameLen = rNameEnd - rNameStart;
                char rVarName[32];
                if (rNameLen > 31) rNameLen = 31;
                strncpy(rVarName, rNameStart, rNameLen);
                rVarName[rNameLen] = '\0';
                
                if (rNameLen == 2 && isDigit(rVarName[1])) {
                    char varType = rVarName[0];
                    int idx = rVarName[1] - '0';
                    
                    if (varType == 'v' && idx < MAX_UINT_VARS) {
                        rightVal = (int32_t)_ctx.uintVars[idx];
                        rightParsed = true;
                    } else if (varType == 'i' && idx < MAX_INT_VARS) {
                        rightVal = _ctx.intVars[idx];
                        rightParsed = true;
                    } else if (varType == 'f' && idx < MAX_FLOAT_VARS) {
                        rightVal = (int32_t)_ctx.floatVars[idx];
                        rightParsed = true;
                    } else if (varType == 's' && idx < MAX_STRING_VARS) {
                        rightVal = 0;
                        rightParsed = true;
                    }
                }
                
                if (!rightParsed) {
                    if (_dataProvider) {
                        DataValue dv;
                        char fullVarName[33];
                        snprintf(fullVarName, 33, "$%s", rVarName);
                        if (_dataProvider(fullVarName, KIND_INT, dv, false)) {
                            rightVal = dv.intVal;
                            rightParsed = true;
                        }
                    }
                }
                p = rNameEnd;
            } else if (isDigit(*p) || *p == '.') {
                rightVal = parseInt(&p);
                rightParsed = true;
            }
            
            if (!rightParsed) {
                if (_logProvider) _logProvider("[Runner] ERROR: Failed to parse right part");
                return false;
            }
            
            int32_t result = 0;
            switch (op) {
                case '+': result = leftVal + rightVal; break;
                case '-': result = leftVal - rightVal; break;
                case '*': result = leftVal * rightVal; break;
                case '/': 
                    if (rightVal != 0) result = leftVal / rightVal; 
                    else {
                        if (_logProvider) _logProvider("[Runner] ERROR: Division by zero");
                        return false;
                    }
                    break;
                default:
                    if (_logProvider) {
                        char logBuf[128];
                        snprintf(logBuf, 128, "[Runner] ERROR: Unknown operator: %c", op);
                        _logProvider(logBuf);
                    }
                    return false;
            }
            
            if (_logProvider) {
                char logBuf[128];
                snprintf(logBuf, 128, "[Runner] External expression result: %d %c %d = %d", leftVal, op, rightVal, result);
                _logProvider(logBuf);
            }
            
            DataValue dv;
            dv.intVal = result;
            if (_dataProvider(name, KIND_INT, dv, true)) {
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] External set: %s = %d", name, result);
                    _logProvider(logBuf);
                }
                return true;
            }
        }
        
        else if (*right == '$') {
            const char* p = right;
            char varType = p[1];
            p += 2;
            int varIndex = parseUint(&p);
            
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
                if (_dataProvider(name, kind, dv, true)) {
                    if (_logProvider) {
                        char logBuf[128];
                        if (kind == KIND_STRING) {
                            snprintf(logBuf, 128, "[Runner] External set: %s = %s", name, _ctx.stringVars[varIndex]);
                        } else if (kind == KIND_FLOAT) {
                            snprintf(logBuf, 128, "[Runner] External set: %s = %.2f", name, dv.floatVal);
                        } else {
                            snprintf(logBuf, 128, "[Runner] External set: %s = %d", name, dv.intVal);
                        }
                        _logProvider(logBuf);
                    }
                    return true;
                }
            }
            return false;
        }
        
        else if (isDigit(*right) || *right == '.') {
            const char* p = right;
            int32_t value = parseInt(&p);
            DataValue dv;
            dv.intVal = value;
            if (_dataProvider(name, KIND_INT, dv, true)) {
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] External set: %s = %d", name, value);
                    _logProvider(logBuf);
                }
                return true;
            }
            return false;
        }
        
        if (_logProvider) {
            char logBuf[128];
            snprintf(logBuf, 128, "[Runner] External variable: no handler for %s = %s", name, right);
            _logProvider(logBuf);
        }
        
        return false;
    }

    return false;
}

bool ScriptRunner::processCommand(const char* token, ScriptState& s, uint32_t now) {
    const char* open = strchr(token, '(');
    if (!open) return false;
    
    const char* eq = strchr(token, '=');
    char varName[16] = {0};
    char varType = 0;
    int varIndex = 0;
    bool hasVar = false;
    
    if (eq) {
        const char* varStart = token;
        const char* varEnd = eq;
        int len = varEnd - varStart;
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
    int len = open - cmdStart;
    if (len > 15) len = 15;
    strncpy(cmd, cmdStart, len);
    cmd[len] = '\0';
    
    Params params = parseParams(token);
    
    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] processCommand: %s", cmd);
        _logProvider(buf);
    }
    
    bool result = false;
    if (strcmp(cmd, "call") == 0) result = handleCall(params, s);
    else if (strcmp(cmd, "on") == 0) result = handleOn(params, s);
    else if (strcmp(cmd, "wait") == 0) result = handleWait(params, s, now);
    else if (strcmp(cmd, "set") == 0) result = handleSet(params, s);
    else if (strcmp(cmd, "get") == 0) result = handleGet(params, s);
    else if (strcmp(cmd, "len") == 0) result = handleLen(params, s);
    else if (strcmp(cmd, "chr") == 0) result = handleChr(params, s);
    else if (strcmp(cmd, "ord") == 0) result = handleOrd(params, s);
    
    if (result && s.hasTempResult && hasVar) {
        if (_logProvider) {
            char logBuf[128];
            snprintf(logBuf, 128, "[Runner] Saving result %d to %s", s.tempResult, varName);
            _logProvider(logBuf);
        }
        
        if (varType == 'v') {
            if (varIndex < MAX_UINT_VARS) {
                _ctx.uintVars[varIndex] = (uint32_t)s.tempResult;
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] $v%d = %d", varIndex, s.tempResult);
                    _logProvider(logBuf);
                }
            }
        } else if (varType == 'i') {
            if (varIndex < MAX_INT_VARS) {
                _ctx.intVars[varIndex] = s.tempResult;
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] $i%d = %d", varIndex, s.tempResult);
                    _logProvider(logBuf);
                }
            }
        } else if (varType == 'f') {
            if (varIndex < MAX_FLOAT_VARS) {
                _ctx.floatVars[varIndex] = (double)s.tempResult;
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] $f%d = %d", varIndex, s.tempResult);
                    _logProvider(logBuf);
                }
            }
        } else if (varType == 's') {
            if (varIndex < MAX_STRING_VARS) {
                char buf[2] = {(char)s.tempResult, '\0'};
                strcpy(_ctx.stringVars[varIndex], buf);
                if (_logProvider) {
                    char logBuf[128];
                    snprintf(logBuf, 128, "[Runner] $s%d = '%c'", varIndex, (char)s.tempResult);
                    _logProvider(logBuf);
                }
            }
        }
        s.hasTempResult = false;
    }
    
    return result;
}

bool ScriptRunner::processToken(const char* token, ScriptState& s, uint32_t now) {
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
        int len = colon - token;
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

    if (_logProvider) {
        char buf[64];
        snprintf(buf, 64, "[Runner] Unknown token: %s", token);
        _logProvider(buf);
    }

    return false;
}

void ScriptRunner::update() {
    if (_queueCount > 0) {
        for (int i = 0; i < MAX_ACTIVE_SCRIPTS; i++) {
            if (!_active[i].active && _queueCount > 0) {
                uint8_t id = _queueId[_queueHead];
                const char* script = _queueScript[_queueHead];
                uint16_t len = _queueLen[_queueHead];
                _queueHead = (_queueHead + 1) % MAX_QUEUE_SIZE;
                _queueCount--;
                activateSlot(i, id, script, len);
            }
        }
    }

    uint32_t now = millis();

    for (int i = 0; i < _activeCount; i++) {
        uint8_t idx = _activeList[i];
        ScriptState& s = _active[idx];

        if (!s.active) {
            removeFromActiveList(idx);
            i--;
            continue;
        }

        if (!s.isInfinite) {
            if (now - s.startTime > 60000) {
                s.active = false;
                removeFromActiveList(idx);
                i--;
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

        if (s.inEvent) {
            if (s.eventTimeout > 0 && (now - s.eventStartTime) > s.eventTimeout) {
                s.inEvent = false;
            } else {
                continue;
            }
        }

        if (now - s.lastExecutionTime < 10) {
            continue;
        }
        s.lastExecutionTime = now;

        if (s.pos >= s.scriptLen) {
            if (s.inLoop) {
                if (s.isInfinite) {
                    s.pos = s.loopStartPos;
                    continue;
                }
                if (s.repeatCount > 0) {
                    s.repeatCount--;
                    if (s.repeatCount > 0) {
                        s.pos = s.loopStartPos;
                        continue;
                    }
                }
                s.inLoop = false;
            }
            s.active = false;
            removeFromActiveList(idx);
            i--;
            continue;
        }

        const char* p = s.script + s.pos;

        while (*p == ',' || *p == ' ') {
            p++;
            s.pos++;
            if (s.pos >= s.scriptLen) break;
        }

        if (s.pos >= s.scriptLen) {
            continue;
        }

        const char* start = p;
        int parenCount = 0;
        while (*p && (p - s.script) < s.scriptLen) {
            if (*p == '(') parenCount++;
            if (*p == ')') parenCount--;
            if (*p == ',' && parenCount == 0) break;
            p++;
        }

        size_t len = p - start;
        if (len >= MAX_TOKEN_LEN) len = MAX_TOKEN_LEN - 1;

        char token[MAX_TOKEN_LEN];
        strncpy(token, start, len);
        token[len] = '\0';
        s.pos = p - s.script;

        if (_logProvider) {
            char buf[128];
            snprintf(buf, 128, "[Runner] Token: %s", token);
            _logProvider(buf);
        }

        processToken(token, s, now);
    }
}

void ScriptRunner::emitEvent(uint32_t hash) {
    for (int i = 0; i < _activeCount; i++) {
        ScriptState& s = _active[_activeList[i]];
        if (s.inEvent && s.eventHash == hash) {
            s.inEvent = false;
            if (_logProvider) {
                char buf[64];
                snprintf(buf, 64, "[Runner] Event emitted: %u", hash);
                _logProvider(buf);
            }
        }
    }
}

void ScriptRunner::emitEvent(const char* eventName) {
    emitEvent(hashEvent(eventName));
}

void ScriptRunner::setOutput(uint8_t gpio, uint16_t value) {
    if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;
    if (_portProvider) {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, 64, "[Runner] setOutput: gpio=%d, value=%d", gpio, value);
            _logProvider(buf);
        }
        _portProvider(gpio, PORT_WRITE, value);
        if (_stateChangeProvider) {
            _stateChangeProvider(gpio, 0, value);
            if (_logProvider) {
                char buf[64];
                snprintf(buf, 64, "[Runner] StateChange: gpio=%d, old=0, new=%d", gpio, value);
                _logProvider(buf);
            }
        }
    } else {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, 64, "[Runner] ERROR: portProvider is NULL! gpio=%d", gpio);
            _logProvider(buf);
        }
    }
}

void ScriptRunner::setError(const char* msg) {
    if (_logProvider) {
        char buf[128];
        snprintf(buf, 128, "[Runner] ERROR: %s", msg);
        _logProvider(buf);
    }
}

ScriptRunner scriptRunner;