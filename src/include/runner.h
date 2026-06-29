#ifndef RUNNER_H
#define RUNNER_H

#include <Arduino.h>

#define MAX_ACTIVE_SCRIPTS 4
#define QUEUE_SIZE 4
#define MAX_TOKEN_LEN 64
#define MAX_PWM_VALUE 255
#define MAX_DATA_SOURCES 16
#define MAX_UINT_VARS 5

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

enum DataType : uint8_t {
  DATA_INT = 0,
  DATA_FLOAT = 1,
  DATA_UINT32 = 2,
  DATA_BOOL = 3
};

struct DataSource {
  char id[16];
  DataType type;
  void* ptr;
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
  bool fadeIsPwm;

  bool inIf;
  bool ifResult;
  bool skipElse;
  uint8_t ifDepth;

  bool inWait;
  uint32_t waitUntil;
};

struct TokenHandler {
  const char* name;
  uint8_t priority;
  bool (*canHandle)(const char* token, ScriptState& s);
  bool (*handle)(const char* token, ScriptState& s, uint32_t now);
};

class ScriptRunner {
 public:
  ScriptRunner(ScriptConflict defaultStrategy = RESTART);

  bool addScript(uint8_t id, const char* script, ScriptConflict strategy = RESTART);
  void update();
  bool stopScript(uint8_t id);
  void stopAll();
  bool isRunning(uint8_t id) const;
  bool isBusy() const;

  void addDataSource(const char* id, DataType type, void* ptr);
  bool getDataValue(const char* id, uint32_t& value);

  typedef bool (*DataProvider)(const char* id, DataType& type, uint32_t& value);
  void setDataProvider(DataProvider provider);

  typedef void (*LogProvider)(const char* message);
  void setLogProvider(LogProvider provider);

  typedef void (*PortOutputCallback)(uint8_t gpio, uint16_t value, uint8_t portType);
  void setPortOutputCallback(PortOutputCallback callback);

  typedef void (*StateChangeProvider)(uint8_t gpio, uint16_t oldValue, uint16_t newValue);
  void setStateChangeProvider(StateChangeProvider provider);

  static bool handlePause(const char* token, ScriptState& s, uint32_t now);
  static bool handleWait(const char* token, ScriptState& s, uint32_t now);
  static bool handleLog(const char* token, ScriptState& s, uint32_t now);
  static bool handleIf(const char* token, ScriptState& s, uint32_t now);
  static bool handleElse(const char* token, ScriptState& s, uint32_t now);
  static bool handleEnd(const char* token, ScriptState& s, uint32_t now);
  static bool handleLoopStart(const char* token, ScriptState& s, uint32_t now);
  static bool handleLoopEnd(const char* token, ScriptState& s, uint32_t now);
  static bool handleVariable(const char* token, ScriptState& s, uint32_t now);
  static bool handleGPIO(const char* token, ScriptState& s, uint32_t now);

 private:
  ScriptState _active[MAX_ACTIVE_SCRIPTS];
  const char* _queueScript[QUEUE_SIZE];
  uint8_t _queueId[QUEUE_SIZE];
  uint16_t _queueLen[QUEUE_SIZE];
  uint8_t _queueHead, _queueTail, _queueCount;
  ScriptConflict _defaultStrategy;

  DataSource _dataSources[MAX_DATA_SOURCES];
  uint8_t _dataSourcesCount;

  DataProvider _dataProvider = nullptr;
  LogProvider _logProvider = nullptr;
  StateChangeProvider _stateChangeProvider = nullptr;
  PortOutputCallback _portOutputCallback = nullptr;
  uint32_t _lastStateChangeTime = 0;

  uint32_t _uintVars[MAX_UINT_VARS];

  static ScriptRunner* _instance;

  int findById(uint8_t id) const;
  bool addToQueue(uint8_t id, const char* script, uint16_t len);
  void activateSlot(int idx, uint8_t id, const char* script, uint16_t len);

  void executeToken(const char* token, ScriptState& s);
  void setOutput(uint8_t gpio, uint16_t value, bool isFadeStep, uint8_t portType);

  void startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, uint8_t portType);
  void updateFade(ScriptState& s, uint32_t now);

  bool getDataSourceValue(const char* id, uint32_t& value);
  DataSource* findDataSource(const char* id);

  bool isLoopStart(const char* token);
  bool isLoopEnd(const char* token);
  uint8_t parseLoopCount(const char* token);
  bool isInfiniteLoop(const char* token);

  bool isIfStart(const char* token);
  bool isElse(const char* token);
  bool isEnd(const char* token);
  bool parseCondition(const char* token, ScriptState& s);

  static const TokenHandler _handlers[];
  static bool isPause(const char* token, ScriptState& s);
  static bool isWait(const char* token, ScriptState& s);
  static bool isLog(const char* token, ScriptState& s);
  static bool isIfStart(const char* token, ScriptState& s);
  static bool isElse(const char* token, ScriptState& s);
  static bool isEnd(const char* token, ScriptState& s);
  static bool isLoopStart(const char* token, ScriptState& s);
  static bool isLoopEnd(const char* token, ScriptState& s);
  static bool isVariable(const char* token, ScriptState& s);
  static bool isGPIO(const char* token, ScriptState& s);

  bool processToken(const char* token, ScriptState& s, uint32_t now);
};

#endif