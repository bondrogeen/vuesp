#ifdef ARDUINO
#include "./runner.h"
#else
#include "runner.h"
#endif

ScriptRunner* ScriptRunner::_instance = nullptr;

uint32_t ScriptRunner::hash(const char* str) {
    if (!str) return 0;
    uint32_t hash = 5381;
    while (*str) {
        hash = ((hash << 5) + hash) + *str++;
    }
    return hash;
}

bool ScriptRunner::onEvent(uint32_t hash, uint8_t slotId) {
    for (int i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) {
            return true;
        }
    }
    if (_eventHandlerCount >= MAX_EVENT_HANDLERS) {
        if (_logProvider) _logProvider("[Runner] ERROR: Too many event handlers!");
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
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] Emitting event: hash=%u", hash);
        _logProvider(buf);
    }
    int calledCount = 0;
    for (int i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) {
            if (_logProvider) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[Runner] Executing handler slot %d", _eventHandlers[i].slotId);
                _logProvider(buf);
            }
            if (runScriptFrom(_eventHandlers[i].slotId, 0, _slots[_eventHandlers[i].slotId].scriptLen)) {
                calledCount++;
            }
        }
    }
    if (_logProvider && calledCount == 0) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] No handlers found for event hash=%u", hash);
        _logProvider(buf);
    }
}

void ScriptRunner::emitEvent(const char* eventName) {
    emitEvent(ScriptRunner::hash(eventName));
}

bool ScriptRunner::removeEventHandler(uint32_t hash) {
    for (int i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].hash == hash) {
            _eventHandlers[i].active = false;
            int slotId = _eventHandlers[i].slotId;
            if (slotId < TOTAL_SLOTS) {
                resetScriptState(slotId);
            }
            if (_logProvider) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[Runner] Event handler removed: hash=%u", hash);
                _logProvider(buf);
            }
            return true;
        }
    }
    return false;
}

void ScriptRunner::clearAllEventHandlers() {
    for (int i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active) {
            int slotId = _eventHandlers[i].slotId;
            if (slotId < TOTAL_SLOTS) {
                resetScriptState(slotId);
            }
        }
    }
    _eventHandlerCount = 0;
    for (int i = 0; i < MAX_EVENT_HANDLERS; i++) {
        _eventHandlers[i].active = false;
        _eventHandlers[i].hash = 0;
        _eventHandlers[i].slotId = 0;
    }
    if (_logProvider) _logProvider("[Runner] All event handlers cleared");
}

void ScriptRunner::initSlotPools() {
    for (int i = 0; i < MAX_SCRIPTS; i++) {
        _slots[i].slotSize = MAX_SCRIPT_LEN;
    }
}

bool ScriptRunner::handleOn(const Params& params, ScriptState& s, uint32_t now) {
    if (params.count < 1) return false;
    
    if (s.inEventHandler) {
        setError("Nested events are not allowed!");
        return false;
    }
    
    const char* rawEventName = params.values[0];
    char cleanEventName[MAX_EVENT_NAME_LEN] = {0};
    const char* eventName = rawEventName;
    
    int len = strlen(rawEventName);
    if (len > 2) {
        char first = rawEventName[0];
        char last = rawEventName[len - 1];
        if ((first == '\'' || first == '"') && (last == '\'' || last == '"')) {
            int cleanLen = len - 2;
            if (cleanLen >= MAX_EVENT_NAME_LEN) cleanLen = MAX_EVENT_NAME_LEN - 1;
            strncpy(cleanEventName, rawEventName + 1, cleanLen);
            cleanEventName[cleanLen] = '\0';
            eventName = cleanEventName;
        }
    }
    
    uint32_t eventHash = ScriptRunner::hash(eventName);
    
    for (int i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].hash == eventHash) {
            if (_logProvider) {
                char buf[128];
                snprintf(buf, sizeof(buf), "[Runner] Event '%s' already registered", eventName);
                _logProvider(buf);
            }
            return true;
        }
    }
    
    const char* p = s.script + s.pos;
    const char* bodyStart = p;
    int depth = 0;
    int bodyLen = 0;
    
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
        setError("Empty event handler body");
        return false;
    }
    
    char tempBody[MAX_SCRIPT_LEN];
    strncpy(tempBody, bodyStart, bodyLen);
    tempBody[bodyLen] = '\0';
    
    char* start = tempBody;
    while (*start == ',' || *start == ' ') start++;
    int end = strlen(start);
    while (end > 0 && (start[end-1] == ',' || start[end-1] == ' ')) {
        end--;
    }
    start[end] = '\0';
    if (start != tempBody) {
        memmove(tempBody, start, end + 1);
    }
    
    int writePos = 0;
    for (int i = 0; tempBody[i]; i++) {
        unsigned char c = (unsigned char)tempBody[i];
        if (c >= 32 && c <= 126) {
            tempBody[writePos++] = c;
        }
    }
    tempBody[writePos] = '\0';
    
    char result[MAX_SCRIPT_LEN] = {0};
    bool first = true;
    char* token = strtok(tempBody, ",");
    while (token) {
        while (*token == ' ') token++;
        int tLen = strlen(token);
        while (tLen > 0 && token[tLen-1] == ' ') {
            token[tLen-1] = '\0';
            tLen--;
        }
        if (strlen(token) > 0) {
            if (!first) strcat(result, ",");
            strcat(result, token);
            first = false;
        }
        token = strtok(NULL, ",");
    }
    
    if (strlen(result) == 0) {
        setError("Empty body after cleaning");
        return false;
    }
    
    uint16_t scriptLen = strlen(result);
    int slot = findFreeSlot();
    if (slot == -1) {
        setError("No free slot for event handler");
        return false;
    }
    
    resetScriptState(slot);
    strcpy(_slots[slot].script, result);
    _slots[slot].registered = true;
    _slots[slot].id = EVENT_HANDLER_ID_BASE + slot;
    _slots[slot].scriptLen = scriptLen;
    _slots[slot].pos = 0;
    _slots[slot].active = false;
    _slots[slot].inEventHandler = false;
    _slots[slot].isHandler = true;
    _slots[slot].isPersistent = true;
    
    if (!onEvent(eventHash, (uint8_t)slot)) {
        resetScriptState(slot);
        return false;
    }
    
    if (_logProvider) {
        char buf[256];
        snprintf(buf, sizeof(buf), "[Runner] Event handler registered: '%s' -> slot %d (id=%d, size=%d, len=%d)", 
                 eventName, slot, _slots[slot].id, _slots[slot].slotSize, scriptLen);
        _logProvider(buf);
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

ScriptRunner::ScriptRunner()
    : _dataProvider(nullptr)
    , _logProvider(nullptr)
    , _portProvider(nullptr)
    , _stateChangeProvider(nullptr)
    , _loadProvider(nullptr)
    , _eventHandlerCount(0) {
    
    _instance = this;
    if (_logProvider) _logProvider("[Runner] Constructor called");
    
    initSlotPools();
    
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        resetScriptState(i);
    }
    
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
    
    for (int i = 0; i < MAX_EVENT_HANDLERS; i++) {
        _eventHandlers[i].active = false;
        _eventHandlers[i].hash = 0;
        _eventHandlers[i].slotId = 0;
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

void ScriptRunner::setLoadProvider(LoadProvider provider) { 
    _loadProvider = provider; 
    if (_logProvider) _logProvider("[Runner] LoadProvider set");
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
        snprintf(buf, sizeof(buf), "[Runner] setUintVar[%d]=%u", idx, value);
        _logProvider(buf);
    }
}

void ScriptRunner::setIntVar(uint8_t idx, int32_t value) {
    if (idx < MAX_INT_VARS) _ctx.intVars[idx] = value;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] setIntVar[%d]=%d", idx, value);
        _logProvider(buf);
    }
}

void ScriptRunner::setFloatVar(uint8_t idx, float value) {
    if (idx < MAX_FLOAT_VARS) _ctx.floatVars[idx] = (double)value;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] setFloatVar[%d]=%.2f", idx, value);
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

int ScriptRunner::findSlotById(uint8_t id) const {
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        if (_slots[i].registered && _slots[i].id == id) {
            return i;
        }
    }
    return -1;
}

int ScriptRunner::findFreeSlot() const {
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        if (!_slots[i].registered) {
            return i;
        }
    }
    return -1;
}

bool ScriptRunner::removeScript(uint8_t id) {
    int slot = findSlotById(id);
    if (slot == -1) {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] Script %d not found for removal", id);
            _logProvider(buf);
        }
        return false;
    }
    
    if (_slots[slot].active) {
        _slots[slot].active = false;
    }
    
    resetScriptState(slot);
    
    for (int i = 0; i < _eventHandlerCount; i++) {
        if (_eventHandlers[i].active && _eventHandlers[i].slotId == slot) {
            _eventHandlers[i].active = false;
            if (_logProvider) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[Runner] Event handler removed with script %d", id);
                _logProvider(buf);
            }
        }
    }
    
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] Script %d removed from slot %d", id, slot);
        _logProvider(buf);
    }
    return true;
}

bool ScriptRunner::registerScript(uint8_t id, const char* script, bool persistent) {
    if (id >= EVENT_HANDLER_ID_BASE) {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] ERROR: ID %d is reserved for event handlers (>=%d)", id, EVENT_HANDLER_ID_BASE);
            _logProvider(buf);
        }
        return false;
    }
    
    uint16_t len = strlen(script);
    if (len >= MAX_SCRIPT_LEN) {
        if (_logProvider) _logProvider("[Runner] ERROR: Script too long!");
        return false;
    }
    
    int existing = findSlotById(id);
    if (existing != -1) {
        if (_slots[existing].isHandler) {
            if (_logProvider) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[Runner] ERROR: ID %d is used by event handler in slot %d!", id, existing);
                _logProvider(buf);
            }
            return false;
        }
        
        strcpy(_slots[existing].script, script);
        _slots[existing].scriptLen = len;
        _slots[existing].pos = 0;
        _slots[existing].active = false;
        _slots[existing].inEventHandler = false;
        _slots[existing].isHandler = false;
        _slots[existing].isPersistent = persistent;
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] Script %d updated in slot %d (len: %d)", 
                     id, existing, len);
            _logProvider(buf);
        }
        return true;
    }
    
    int slot = findFreeSlot();
    if (slot == -1) {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] ERROR: No free slots!");
            _logProvider(buf);
        }
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
    
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] Script %d registered in slot %d (len: %d)", 
                 id, slot, len);
        _logProvider(buf);
    }
    return true;
}

bool ScriptRunner::runScript(uint8_t id) {
    if (id >= EVENT_HANDLER_ID_BASE) {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] ERROR: Cannot run event handler (id=%d)", id);
            _logProvider(buf);
        }
        return false;
    }
    
    int slot = findSlotById(id);
    if (slot != -1) {
        if (_slots[slot].isHandler) {
            if (_logProvider) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[Runner] ERROR: Slot %d is an event handler, not a script!", slot);
                _logProvider(buf);
            }
            return false;
        }
        
        if (_slots[slot].active) {
            if (_logProvider) {
                char buf[64];
                snprintf(buf, sizeof(buf), "[Runner] Script %d already running, RESTART", id);
                _logProvider(buf);
            }
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
        
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] Script %d started", id);
            _logProvider(buf);
        }
        return true;
    }
    
    if (_loadProvider) {
        char buffer[MAX_SCRIPT_LEN];
        uint16_t len = 0;
        
        if (_loadProvider(id, buffer, len)) {
            if (len > 0 && len < MAX_SCRIPT_LEN) {
                buffer[len] = '\0';
                if (registerScript(id, buffer, true)) {
                    if (_logProvider) {
                        char buf[64];
                        snprintf(buf, sizeof(buf), "[Runner] Script %d loaded and registered", id);
                        _logProvider(buf);
                    }
                    return runScript(id);
                }
            }
        }
    }
    
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] ERROR: Script %d not found!", id);
        _logProvider(buf);
    }
    return false;
}

bool ScriptRunner::runScriptFrom(uint8_t slot, uint16_t offset, uint16_t len) {
    if (slot >= TOTAL_SLOTS) {
        if (_logProvider) _logProvider("[Runner] ERROR: Slot out of range!");
        return false;
    }
    
    if (!_slots[slot].registered) {
        if (_logProvider) _logProvider("[Runner] ERROR: Slot not registered!");
        return false;
    }
    
    if (_slots[slot].active) {
        if (_logProvider) {
            char buf[64];
            snprintf(buf, sizeof(buf), "[Runner] Script slot %d already running, RESTART", slot);
            _logProvider(buf);
        }
        _slots[slot].active = false;
    }
    
    if (offset + len > MAX_SCRIPT_LEN) {
        len = MAX_SCRIPT_LEN - offset;
    }
    
    _slots[slot].pos = offset;
    _slots[slot].scriptLen = offset + len;
    _slots[slot].active = true;
    _slots[slot].startTime = millis();
    _slots[slot].lastExecutionTime = 0;
    _slots[slot].inLoop = false;
    _slots[slot].inIf = false;
    _slots[slot].inWait = false;
    _slots[slot].inEventHandler = false;
    
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] Script slot %d started from offset %d (len=%d)", slot, offset, len);
        _logProvider(buf);
    }
    return true;
}

bool ScriptRunner::stopScript(uint8_t id) {
    int slot = findSlotById(id);
    if (slot == -1) return false;
    _slots[slot].active = false;
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] Script %d stopped", id);
        _logProvider(buf);
    }
    return true;
}

void ScriptRunner::stopAll() {
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        _slots[i].active = false;
    }
    if (_logProvider) _logProvider("[Runner] All scripts stopped");
}

bool ScriptRunner::isRunning(uint8_t id) const {
    int slot = findSlotById(id);
    if (slot == -1) return false;
    return _slots[slot].active;
}

bool ScriptRunner::isBusy() const {
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        if (_slots[i].active) return true;
    }
    return false;
}

bool ScriptRunner::isSlotUsed(uint8_t slot) const {
    if (slot >= TOTAL_SLOTS) return false;
    return _slots[slot].registered;
}

int ScriptRunner::getSlotId(uint8_t slot) const {
    if (slot >= TOTAL_SLOTS) return -1;
    if (!_slots[slot].registered) return -1;
    return _slots[slot].id;
}

bool ScriptRunner::isSlotActive(uint8_t slot) const {
    if (slot >= TOTAL_SLOTS) return false;
    return _slots[slot].active;
}

bool ScriptRunner::isSlotHandler(uint8_t slot) const {
    if (slot >= TOTAL_SLOTS) return false;
    return _slots[slot].isHandler;
}

uint16_t ScriptRunner::getSlotLen(uint8_t slot) const {
    if (slot >= TOTAL_SLOTS) return 0;
    return _slots[slot].scriptLen;
}

uint16_t ScriptRunner::getSlotSize(uint8_t slot) const {
    if (slot >= TOTAL_SLOTS) return 0;
    return _slots[slot].slotSize;
}

uint8_t ScriptRunner::getTotalSlots() const {
    return TOTAL_SLOTS;
}

uint8_t ScriptRunner::getUsedSlotsCount() const {
    uint8_t count = 0;
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        if (_slots[i].registered) count++;
    }
    return count;
}

uint8_t ScriptRunner::getFreeSlotsCount() const {
    return TOTAL_SLOTS - getUsedSlotsCount();
}

uint32_t ScriptRunner::getTotalMemory() const {
    uint32_t total = 0;
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        total += _slots[i].slotSize;
    }
    return total;
}

uint32_t ScriptRunner::getUsedMemory() const {
    uint32_t used = 0;
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        if (_slots[i].registered) {
            used += _slots[i].slotSize;
        }
    }
    return used;
}

uint32_t ScriptRunner::getFreeMemory() const {
    return getTotalMemory() - getUsedMemory();
}

void ScriptRunner::getSlotInfo(uint8_t slot, uint8_t& id, uint16_t& size, uint16_t& used, bool& active, bool& isHandler) const {
    if (slot >= TOTAL_SLOTS) {
        id = 0;
        size = 0;
        used = 0;
        active = false;
        isHandler = false;
        return;
    }
    
    id = _slots[slot].id;
    size = _slots[slot].slotSize;
    used = _slots[slot].scriptLen;
    active = _slots[slot].active;
    isHandler = _slots[slot].isHandler;
}

void ScriptRunner::printSlotInfo() const {
    if (!_logProvider) return;
    
    _logProvider("[Runner] ============ SLOT INFO ============");
    
    char buf[256];
    uint8_t usedSlots = 0;
    uint32_t usedMem = 0;
    uint32_t totalMem = 0;
    
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        const ScriptState& s = _slots[i];
        totalMem += s.slotSize;
        
        if (s.registered) {
            usedSlots++;
            usedMem += s.slotSize;
            const char* type = s.isHandler ? "HANDLER" : "SCRIPT";
            const char* status = s.active ? "ACTIVE" : "IDLE";
            snprintf(buf, sizeof(buf), "  Slot %2d: ID=%3d | %s | %s | size=%3d | used=%3d | '%.50s'", 
                     i, s.id, type, status, s.slotSize, s.scriptLen, s.script);
        } else {
            snprintf(buf, sizeof(buf), "  Slot %2d: FREE  | size=%3d", i, s.slotSize);
        }
        _logProvider(buf);
    }
    
    snprintf(buf, sizeof(buf), "  TOTAL: slots=%d/%d, memory=%d/%d bytes (%.1f%%)", 
             usedSlots, TOTAL_SLOTS, usedMem, totalMem, 
             totalMem > 0 ? (float)usedMem / totalMem * 100 : 0);
    _logProvider(buf);
    _logProvider("[Runner] =====================================");
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
    int len = *p - start;
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
        default:  return value * 1000;
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

bool ScriptRunner::handleCall(const Params& params, ScriptState& s) {
    if (params.count < 1) return false;
    uint8_t id = atoi(params.values[0]);
    if (_logProvider) {
        char buf[64];
        snprintf(buf, sizeof(buf), "[Runner] handleCall: %d", id);
        _logProvider(buf);
    }
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
        setError("Nested loops are not allowed");
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
                    return true;
                }
            } else if (strchr(right, '+') != nullptr) {
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
                return true;
            } else if (*right == '$' && right[1] == 's') {
                right += 2;
                int srcIdx = parseUint(&right);
                if (srcIdx < MAX_STRING_VARS) {
                    strcpy(_ctx.stringVars[index], _ctx.stringVars[srcIdx]);
                    return true;
                }
            } else if (s.hasTempResult) {
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
            } else if (*p2 == '$') {
                if (p2[1] == 'f') {
                    const char* temp = p2 + 2;
                    int idx = parseUint(&temp);
                    if (idx < MAX_FLOAT_VARS) {
                        leftFloatVal = (float)_ctx.floatVars[idx];
                        leftIsFloat = true;
                        hasLeft = true;
                        p2 = temp;
                    }
                } else if (p2[1] == 'p') {
                    const char* temp = p2 + 2;
                    int gpio = parseUint(&temp);
                    uint16_t val = 0;
                    if (_portProvider && _portProvider(gpio, PORT_READ, val)) {
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
                    return true;
                }
            }
            return false;
        } else if (*right == '$') {
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
                    return true;
                }
            }
            return false;
        } else if (isDigit(*right) || *right == '.') {
            const char* p = right;
            int32_t value = parseInt(&p);
            DataValue dv;
            dv.intVal = value;
            if (_dataProvider(name, KIND_INT, dv, true)) {
                return true;
            }
            return false;
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
    s.pos++;
    return false;
}

void ScriptRunner::update() {
    uint32_t now = millis();
    for (int i = 0; i < TOTAL_SLOTS; i++) {
        ScriptState& s = _slots[i];
        if (!s.active || !s.registered) continue;
        if (s.inWait) {
            if (now >= s.waitUntil) {
                s.inWait = false;
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
            if (!s.isHandler && !s.isPersistent) {
                if (_logProvider) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "[Runner] Auto-removing script %d (slot %d)", s.id, i);
                    _logProvider(buf);
                }
                resetScriptState(i);
            } else {
                s.active = false;
                if (_logProvider) {
                    char buf[64];
                    snprintf(buf, sizeof(buf), "[Runner] Script %d completed (kept)", s.id);
                    _logProvider(buf);
                }
            }
            continue;
        }
        const char* p = s.script + s.pos;
        while (*p == ' ' || *p == ',' || *p == '\t' || *p == '\r' || *p == '\n' || *p < 32) {
            p++;
            s.pos++;
            if (s.pos >= s.scriptLen) break;
        }
        if (s.pos >= s.scriptLen) {
            s.active = false;
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
        if (len == 0 || token[0] == '\0' || token[0] == ' ' || token[0] == ',') {
            s.pos++;
            continue;
        }
        processToken(token, s, now);
    }
}

void ScriptRunner::setOutput(uint8_t gpio, uint16_t value) {
    if (value > MAX_PWM_VALUE) value = MAX_PWM_VALUE;
    if (_portProvider) {
        _portProvider(gpio, PORT_WRITE, value);
        if (_stateChangeProvider) {
            _stateChangeProvider(gpio, 0, value);
        }
    }
}

void ScriptRunner::setError(const char* msg) {
    if (_logProvider) {
        char buf[128];
        snprintf(buf, sizeof(buf), "[Runner] ERROR: %s", msg);
        _logProvider(buf);
    }
}

ScriptRunner scriptRunner;