#ifndef RUNNER_H
#define RUNNER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "../tests/arduino_stub.h"
#endif

// ============================================
// КОНСТАНТЫ
// ============================================

#define MAX_ACTIVE_SCRIPTS 10
#define MAX_QUEUE_SIZE 10
#define MAX_TOKEN_LEN 80
#define MAX_SCRIPT_LEN 255

#define MAX_UINT_VARS 10   // $v0..$v9
#define MAX_INT_VARS 10    // $i0..$i9
#define MAX_FLOAT_VARS 5   // $f0..$f4
#define MAX_STRING_VARS 2  // $s0..$s1
#define MAX_ARRAY_VARS 5   // $a0..$a4
#define MAX_ARRAY_SIZE 64
#define MAX_STRING_LEN 64

#define MAX_REGISTERED 20
#define MAX_PWM_VALUE 255
#define MAX_PARAMS 8

// ============================================
// ТИПЫ
// ============================================

enum ScriptConflict : uint8_t {
  RESTART = 0,
  IGNORE = 1,
  RESTART_IF_SAME = 2,
  ADD_QUEUE = 3
};

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

// ============================================
// КОНТЕКСТ
// ============================================

struct ScriptContext {
  uint32_t uintVars[MAX_UINT_VARS];                  // $v0..$v9
  int32_t intVars[MAX_INT_VARS];                     // $i0..$i9
  double floatVars[MAX_FLOAT_VARS];                  // $f0..$f4
  char stringVars[MAX_STRING_VARS][MAX_STRING_LEN];  // $s0..$s1

  // Массивы
  uint8_t arrayVars[MAX_ARRAY_VARS][MAX_ARRAY_SIZE];
  uint8_t arrayLen[MAX_ARRAY_VARS];
};

// ============================================
// ПАРАМЕТРЫ
// ============================================

struct Params {
  char values[MAX_PARAMS][32];
  int count;
};

// ============================================
// СОСТОЯНИЕ СКРИПТА
// ============================================

struct ScriptState {
  bool active;
  uint8_t id;
  const char* script;
  uint16_t pos;
  uint16_t scriptLen;
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
  const char* waitEvent;
  uint32_t waitEventHash;

  bool inEvent;
  uint32_t eventHash;
  uint32_t eventStartTime;
  uint32_t eventTimeout;

  // Временная переменная для результатов функций
  int32_t tempResult;
  bool hasTempResult;
  
  // ⭐ ДЛЯ WHILE
  bool isWhile;
  const char* whileCondition;
  char whileConditionBuffer[64];
};

struct ScriptEntry {
  uint8_t id;
  const char* script;
};

// ============================================
// ПРОВАЙДЕРЫ
// ============================================

typedef bool (*DataProvider)(const char* id, DataKind kind, DataValue& value, bool write);
typedef void (*LogProvider)(const char* message);
typedef bool (*PortProvider)(uint8_t gpio, PortAction action, uint16_t& value);
typedef void (*StateChangeProvider)(uint8_t gpio, uint16_t oldValue, uint16_t newValue);

// ============================================
// SCRIPTRUNNER
// ============================================

class ScriptRunner {
 public:
  ScriptRunner(ScriptConflict defaultStrategy = RESTART);
  ~ScriptRunner();

  bool registerScript(uint8_t id, const char* script);
  bool addScript(uint8_t id, const char* script, ScriptConflict strategy = RESTART);
  bool runScript(uint8_t id);
  void update();
  bool stopScript(uint8_t id);
  void stopAll();
  bool isRunning(uint8_t id) const;
  bool isBusy() const;

  uint32_t getUintVar(uint8_t idx) const;
  int32_t getIntVar(uint8_t idx) const;
  float getFloatVar(uint8_t idx) const;
  void setUintVar(uint8_t idx, uint32_t value);
  void setIntVar(uint8_t idx, int32_t value);
  void setFloatVar(uint8_t idx, float value);

  // Массивы
  uint8_t getArrayByte(uint8_t idx, uint8_t pos) const;
  void setArrayByte(uint8_t idx, uint8_t pos, uint8_t value);
  uint8_t getArrayLen(uint8_t idx) const;

  void setDataProvider(DataProvider provider);
  void setLogProvider(LogProvider provider);
  void setPortProvider(PortProvider provider);
  void setStateChangeProvider(StateChangeProvider provider);

  void emitEvent(uint32_t hash);
  void emitEvent(const char* eventName);

 private:
  ScriptState _active[MAX_ACTIVE_SCRIPTS];
  uint8_t _activeList[MAX_ACTIVE_SCRIPTS];
  uint8_t _activeCount;

  const char* _queueScript[MAX_QUEUE_SIZE];
  uint8_t _queueId[MAX_QUEUE_SIZE];
  uint16_t _queueLen[MAX_QUEUE_SIZE];
  uint8_t _queueHead, _queueTail, _queueCount;

  ScriptEntry _registry[MAX_REGISTERED];
  uint8_t _registryCount;

  ScriptContext _ctx;

  DataProvider _dataProvider;
  LogProvider _logProvider;
  PortProvider _portProvider;
  StateChangeProvider _stateChangeProvider;
  uint32_t _lastStateChangeTime;

  ScriptConflict _defaultStrategy;
  static ScriptRunner* _instance;

  void resetScriptState(int idx);
  void addToActiveList(uint8_t idx);
  void removeFromActiveList(uint8_t idx);
  int findById(uint8_t id) const;
  int findInRegistry(uint8_t id) const;
  void addToRegistry(uint8_t id, const char* script);
  bool addToQueue(uint8_t id, const char* script, uint16_t len);
  void activateSlot(int idx, uint8_t id, const char* script, uint16_t len);

  Params parseParams(const char* str);
  uint32_t parseTime(const char* str);
  uint32_t parseUint(const char** p);
  int32_t parseInt(const char** p);
  float parseFloat(const char** p);
  bool parseString(const char** p, char* buf);
  bool parseValue(const char** p, ScriptState& s, int32_t& result);
  bool parseArray(const char** p, uint8_t idx);
  uint32_t hashEvent(const char* str) const;

  bool processToken(const char* token, ScriptState& s, uint32_t now);
  bool processCommand(const char* token, ScriptState& s, uint32_t now);

  bool handleCall(const Params& params, ScriptState& s);
  bool handleOn(const Params& params, ScriptState& s);
  bool handleWait(const Params& params, ScriptState& s, uint32_t now);
  bool handleWhile(const char* params, ScriptState& s);
  bool handleIf(const char* params, ScriptState& s);
  bool handleElse(ScriptState& s);
  bool handleEnd(ScriptState& s);
  bool handleAssignment(const char* token, ScriptState& s);

  // Обработчики для массивов и строк
  bool handleSet(const Params& params, ScriptState& s);
  bool handleGet(const Params& params, ScriptState& s);
  bool handleLen(const Params& params, ScriptState& s);
  bool handleChr(const Params& params, ScriptState& s);
  bool handleOrd(const Params& params, ScriptState& s);

  void setOutput(uint8_t gpio, uint16_t value);
  bool parseCondition(const char* token, ScriptState& s);
  void setError(const char* msg);
};

extern ScriptRunner scriptRunner;

#endif