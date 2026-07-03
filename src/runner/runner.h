#ifndef RUNNER_H
#define RUNNER_H

#ifdef ARDUINO
#include <Arduino.h>
#else
#include "../tests/arduino_stub.h"
#endif

#define MAX_ACTIVE_SCRIPTS 10
#define QUEUE_SIZE 10
#define MAX_TOKEN_LEN 80
#define MAX_PWM_VALUE 255
#define MAX_UINT_VARS 10
#define MAX_FLOAT_VARS 5
#define MAX_REGISTERED_SCRIPTS 20
#define MAX_STRING_VARS 2
#define MAX_STRING_LEN 64

#define PORT_TYPE_DIGITAL 0
#define PORT_TYPE_PWM 1
#define PORT_TYPE_ADC 2
#define PORT_TYPE_INPUT 3

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
  KIND_FLOAT,
  KIND_STRING
};

union DataValue {
  uint32_t uintVal;
  float floatVal;
  struct {
    uint8_t* data;
    uint8_t len;
  } stringVal;
};

struct ScriptState {
  bool active;
  uint8_t id;
  const char* script;
  uint16_t pos;
  bool inPause;
  uint32_t pauseUntil;
  uint32_t startTime;
  uint16_t scriptLen;

  bool inLoop;
  bool isInfinite;
  uint8_t repeatCount;
  uint16_t loopStartPos;

  uint32_t lastExecutionTime;

  bool inFade;
  uint8_t fadeGpio;
  uint16_t fadeTarget;
  uint16_t fadeStartValue;
  uint32_t fadeStartTime;
  uint32_t fadeDuration;

  bool inIf;
  bool ifResult;
  bool skipElse;
  uint8_t ifDepth;

  bool inWait;
  uint32_t waitUntil;

  bool inEvent;
  uint32_t eventHash;
};

struct ScriptEntry {
  uint8_t id;
  const char* script;
};

typedef bool (*DataProvider)(const char* id, DataKind kind, DataValue& value, bool write);

class ScriptRunner {
 public:
  ScriptRunner(ScriptConflict defaultStrategy = RESTART);

  bool addScript(uint8_t id, const char* script, ScriptConflict strategy = RESTART);
  bool registerScript(uint8_t id, const char* script);
  bool runScript(uint8_t id);
  void update();
  bool stopScript(uint8_t id);
  void stopAll();
  bool isRunning(uint8_t id) const;
  bool isBusy() const;

  bool getDataValue(const char* id, uint32_t& value);

  uint32_t getUintVar(uint8_t idx) const;
  float getFloatVar(uint8_t idx) const;
  void setUintVar(uint8_t idx, uint32_t value);
  void setFloatVar(uint8_t idx, float value);

  void setDataProvider(DataProvider provider);

  typedef void (*LogProvider)(const char* message);
  void setLogProvider(LogProvider provider);

  typedef bool (*PortProvider)(uint8_t gpio, PortAction action, uint16_t& value);
  void setPortProvider(PortProvider provider);

  typedef void (*StateChangeProvider)(uint8_t gpio, uint16_t oldValue, uint16_t newValue);
  void setStateChangeProvider(StateChangeProvider provider);

  void emitEvent(uint32_t hash);
  void emitEvent(const char* eventName);

  static bool handlePause(const char* token, ScriptState& s, uint32_t now);
  static bool handleWait(const char* token, ScriptState& s, uint32_t now);
  static bool handleLog(const char* token, ScriptState& s, uint32_t now);
  static bool handleIf(const char* token, ScriptState& s, uint32_t now);
  static bool handleElse(const char* token, ScriptState& s, uint32_t now);
  static bool handleEnd(const char* token, ScriptState& s, uint32_t now);
  static bool handleLoopStart(const char* token, ScriptState& s, uint32_t now);
  static bool handleLoopEnd(const char* token, ScriptState& s, uint32_t now);
  static bool handleVariable(const char* token, ScriptState& s, uint32_t now);
  static bool handleCall(const char* token, ScriptState& s, uint32_t now);
  static bool handleOn(const char* token, ScriptState& s, uint32_t now);

 private:
  ScriptState _active[MAX_ACTIVE_SCRIPTS];
  uint8_t _activeList[MAX_ACTIVE_SCRIPTS];
  uint8_t _activeCount;

  const char* _queueScript[QUEUE_SIZE];
  uint8_t _queueId[QUEUE_SIZE];
  uint16_t _queueLen[QUEUE_SIZE];
  uint8_t _queueHead, _queueTail, _queueCount;
  ScriptConflict _defaultStrategy;

  ScriptEntry _registry[MAX_REGISTERED_SCRIPTS];
  uint8_t _registryCount;

  DataProvider _dataProvider = nullptr;
  LogProvider _logProvider = nullptr;
  StateChangeProvider _stateChangeProvider = nullptr;
  PortProvider _portProvider = nullptr;
  uint32_t _lastStateChangeTime = 0;

  uint32_t _uintVars[MAX_UINT_VARS];
  float _floatVars[MAX_FLOAT_VARS];
  uint8_t _stringVars[MAX_STRING_VARS][MAX_STRING_LEN];
  uint8_t _stringLen[MAX_STRING_VARS];

  static ScriptRunner* _instance;

  void resetScriptState(int idx);
  bool checkScriptState(ScriptState& s, uint32_t now);
  void addToActiveList(uint8_t idx);
  void removeFromActiveList(uint8_t idx);

  int findById(uint8_t id) const;
  int findInRegistry(uint8_t id) const;
  void addToRegistry(uint8_t id, const char* script);
  bool addToQueue(uint8_t id, const char* script, uint16_t len);
  void activateSlot(int idx, uint8_t id, const char* script, uint16_t len);

  void setOutput(uint8_t gpio, uint16_t value, bool isFadeStep);

  void startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, uint16_t startValue);
  void updateFade(ScriptState& s, uint32_t now);

  bool resolveValue(const char* p, uint32_t& value);

  uint32_t hashEvent(const char* str) const;
  uint8_t parseLoopCount(const char* token);
  bool isInfiniteLoop(const char* token);
  bool parseCondition(const char* token, ScriptState& s);
  bool tryLoopContinue(ScriptState& s);

  bool processToken(const char* token, ScriptState& s, uint32_t now);
};

extern ScriptRunner scriptRunner;

#endif