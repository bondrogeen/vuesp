// runner.h
#ifndef RUNNER_H
#define RUNNER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "../tests/arduino_stub.h"
#endif

#include <stdint.h>
#include <cstring>

#define ENABLE_LOAD_CACHE 1
#define LOAD_CACHE_SIZE 2

#define ENABLE_PROVIDER_LOGGING 1

#define MAX_SCRIPTS 15
#define MAX_SCRIPT_LEN 256
#define MAX_TOKEN_LEN 48
#define MAX_UINT_VARS 10
#define MAX_INT_VARS 10
#define MAX_FLOAT_VARS 5
#define MAX_STRING_VARS 2
#define MAX_ARRAY_VARS 5
#define MAX_ARRAY_SIZE 64
#define MAX_STRING_LEN 32

#define MAX_PWM_VALUE 255
#define MAX_PARAMS 4
#define MAX_EVENT_HANDLERS 20
#define MAX_EVENT_NAME_LEN 16

#define SCRIPT_ID_BASE 1

#define TOKEN_SEPARATOR ';'
#define ARRAY_SEPARATOR ','

#define SCRIPT_EXEC_INTERVAL_MS 10

enum PortAction : uint8_t {
    PORT_READ = 0,
    PORT_WRITE = 1
};

enum DataKind : uint8_t {
    KIND_UINT,
    KIND_INT,
    KIND_FLOAT,
    KIND_STRING
};

union DataValue {
    uint32_t uintVal;
    int32_t intVal;
    float floatVal;
    struct {
        uint8_t* data;
        uint8_t len;
    } stringVal;
};

struct EventHandler {
    uint32_t hash;
    uint8_t slotId;
    bool active;
};

struct ScriptContext {
    uint32_t uintVars[MAX_UINT_VARS];
    int32_t intVars[MAX_INT_VARS];
    double floatVars[MAX_FLOAT_VARS];
    char stringVars[MAX_STRING_VARS][MAX_STRING_LEN];
    uint8_t arrayVars[MAX_ARRAY_VARS][MAX_ARRAY_SIZE];
    uint8_t arrayLen[MAX_ARRAY_VARS];
};

struct Params {
    char values[MAX_PARAMS][32];
    uint8_t count;
};

struct ScriptState {
    bool active;
    bool registered;
    bool inEventHandler;
    bool isHandler;
    bool isPersistent;
    uint8_t id;
    char script[MAX_SCRIPT_LEN];
    uint16_t scriptLen;
    uint16_t slotSize;
    uint16_t pos;
    uint32_t startTime;
    uint32_t lastExecutionTime;

    bool inLoop;
    bool isInfinite;
    uint8_t repeatCount;
    uint16_t loopStartPos;

    bool inIf;
    bool ifResult;
    bool skipElse;
    uint8_t ifDepth;

    bool inWait;
    uint32_t waitUntil;

    int32_t tempResult;
    bool hasTempResult;
    
    bool isWhile;
    char whileConditionBuffer[32];
};

typedef bool (*DataProvider)(const char* id, DataKind kind, DataValue& value, bool write);
typedef void (*LogProvider)(const char* message);
typedef bool (*PortProvider)(uint8_t gpio, PortAction action, uint16_t& value);
typedef void (*StateChangeProvider)(uint8_t gpio, uint16_t oldValue, uint16_t newValue);
typedef bool (*LoadProvider)(uint8_t id, char* buffer, uint16_t& len);

class ScriptRunner {
public:
    ScriptRunner();
    ~ScriptRunner();

    bool registerScript(uint8_t id, const char* script, bool persistent = false);
    bool runScript(uint8_t id);
    bool runScriptFrom(uint8_t slot, uint16_t offset, uint16_t len);
    void update();
    bool stopScript(uint8_t id);
    void stopAll();
    bool isRunning(uint8_t id) const;
    bool isBusy() const;
    bool removeScript(uint8_t id);

    bool isSlotUsed(uint8_t slot) const;
    int getSlotId(uint8_t slot) const;
    bool isSlotActive(uint8_t slot) const;
    bool isSlotHandler(uint8_t slot) const;
    uint16_t getSlotLen(uint8_t slot) const;
    uint16_t getSlotSize(uint8_t slot) const;

    uint8_t getTotalSlots() const;
    uint8_t getUsedSlotsCount() const;
    uint8_t getFreeSlotsCount() const;
    uint32_t getTotalMemory() const;
    uint32_t getUsedMemory() const;
    uint32_t getFreeMemory() const;

    static uint32_t hash(const char* str);
    
    bool onEvent(uint32_t hash, uint8_t slotId);
    bool onEvent(const char* eventName, uint8_t slotId);
    void emitEvent(uint32_t hash);
    void emitEvent(const char* eventName);
    bool removeEventHandler(uint32_t hash);
    void clearAllEventHandlers();

    uint32_t getUintVar(uint8_t idx) const;
    int32_t getIntVar(uint8_t idx) const;
    float getFloatVar(uint8_t idx) const;
    void setUintVar(uint8_t idx, uint32_t value);
    void setIntVar(uint8_t idx, int32_t value);
    void setFloatVar(uint8_t idx, float value);

    uint8_t getArrayByte(uint8_t idx, uint8_t pos) const;
    void setArrayByte(uint8_t idx, uint8_t pos, uint8_t value);
    uint8_t getArrayLen(uint8_t idx) const;

    void setDataProvider(DataProvider provider);
    void setLogProvider(LogProvider provider);
    void setPortProvider(PortProvider provider);
    void setStateChangeProvider(StateChangeProvider provider);
    void setLoadProvider(LoadProvider provider);

private:
    ScriptState _slots[MAX_SCRIPTS];
    ScriptContext _ctx;
    EventHandler _eventHandlers[MAX_EVENT_HANDLERS];
    uint8_t _eventHandlerCount;

    DataProvider _dataProvider;
    LogProvider _logProvider;
    PortProvider _portProvider;
    StateChangeProvider _stateChangeProvider;
    LoadProvider _loadProvider;

    static ScriptRunner* _instance;

    char _tokenBuf[MAX_TOKEN_LEN];
    char _logBuf[64];
    char _handlerBody[MAX_SCRIPT_LEN];
    char _cleanedBody[MAX_SCRIPT_LEN];
    char _strBuf[MAX_STRING_LEN];
    char _nameBuf[32];

    void resetScriptState(int idx);
    int findSlotById(uint8_t id) const;
    int findFreeSlot(uint16_t scriptLen);
    void initSlotPools();

    Params parseParams(const char* str) const;
    uint32_t parseTime(const char* str) const;
    uint32_t parseUint(const char** p) const;
    int32_t parseInt(const char** p) const;
    float parseFloat(const char** p) const;
    bool parseString(const char** p, char* buf) const;
    bool parseValue(const char** p, ScriptState& s, int32_t& result);
    bool parseArray(const char** p, uint8_t idx);

    bool processToken(const char* token, ScriptState& s, uint32_t now);
    bool processCommand(const char* token, ScriptState& s, uint32_t now);

    bool handleCall(const Params& params, ScriptState& s);
    bool handleOn(const Params& params, ScriptState& s, uint32_t now);
    bool handleWait(const Params& params, ScriptState& s, uint32_t now);
    bool handleWhile(const char* params, ScriptState& s);
    bool handleIf(const char* params, ScriptState& s);
    bool handleElse(ScriptState& s);
    bool handleEnd(ScriptState& s);
    bool handleAssignment(const char* token, ScriptState& s);

    bool handleSet(const Params& params, ScriptState& s);
    bool handleGet(const Params& params, ScriptState& s);
    bool handleLen(const Params& params, ScriptState& s);
    bool handleChr(const Params& params, ScriptState& s);
    bool handleOrd(const Params& params, ScriptState& s);

    void setOutput(uint8_t gpio, uint16_t value);
    bool parseCondition(const char* token, ScriptState& s);
    void setError(const char* msg);

    bool parseVarUint(uint8_t idx, int32_t& result);
    bool parseVarInt(uint8_t idx, int32_t& result);
    bool parseVarFloat(uint8_t idx, int32_t& result);
    bool parseVarString(uint8_t idx, int32_t& result);
    bool parseVarPort(uint8_t idx, int32_t& result);
    bool parseVarData(const char* start, int32_t& result, const char** p);

    void processScript(uint8_t idx, uint32_t now);
    bool getNextToken(ScriptState& s, char* token, uint16_t& tokenLen);
    void finishScript(ScriptState& s, uint8_t idx);

    #ifdef ENABLE_LOAD_CACHE
    struct LoadCacheEntry {
        uint8_t id;
        char script[MAX_SCRIPT_LEN];
        uint16_t len;
        bool valid;
        uint32_t lastAccess;
        uint8_t accessCount;
    };
    
    LoadCacheEntry _loadCache[LOAD_CACHE_SIZE];
    uint32_t _loadCacheHits;
    uint32_t _loadCacheMisses;
    LoadProvider _originalLoadProvider;
    
    int findInLoadCache(uint8_t id, char* buffer, uint16_t& len);
    void addToLoadCache(uint8_t id, const char* script, uint16_t len);
    int findEmptyLoadSlot() const;
    int findLeastUsedSlot() const;
    
    static bool cachedLoadProviderWrapper(uint8_t id, char* buffer, uint16_t& len);
    #endif

    #ifdef ENABLE_PROVIDER_LOGGING
    void logProviderSet(const char* name, bool enabled);
    void logPortAction(uint8_t gpio, PortAction action, uint16_t value);
    void logDataAction(const char* id, DataKind kind, bool write, const char* value);
    void logLoadAction(uint8_t id, uint16_t len, bool cached);
    #endif
};

extern ScriptRunner scriptRunner;

#endif