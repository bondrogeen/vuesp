#ifndef RUNNER_H
#define RUNNER_H

#include <Arduino.h>

#include "./include/gpio.h"
#include "./include/init.h"

#define MAX_ACTIVE_SCRIPTS 4
#define QUEUE_SIZE 4
#define MAX_TOKEN_LEN 32
#define MAX_PWM_VALUE 255
#define MAX_PORTS 16
#define MAX_DATA_SOURCES 16

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

struct PortState {
  uint8_t gpio;
  uint8_t mode;
  uint16_t value;
  bool isOutput;
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
  bool fadeIsToggle;
  uint16_t fadeToggleValue;

  bool inIf;
  bool ifResult;
  bool skipElse;
  uint8_t ifDepth;

  bool inWait;
  uint32_t waitUntil;
};

class ScriptRunner {
 public:
  ScriptRunner(ScriptConflict defaultStrategy = RESTART);

  void initPorts(Port* ports, uint8_t count);
  bool addScript(uint8_t id, const char* script, ScriptConflict strategy = RESTART);
  void update();
  bool stopScript(uint8_t id);
  void stopAll();
  bool isRunning(uint8_t id) const;
  bool isBusy() const;

  bool setPortValue(uint8_t gpio, uint16_t value);
  uint16_t getPortValue(uint8_t gpio) const;
  void syncInputs();

  void addDataSource(const char* id, DataType type, void* ptr);
  bool getDataValue(const char* id, uint32_t& value);

  // ===== DATA PROVIDER =====
  typedef bool (*DataProvider)(const char* id, DataType& type, uint32_t& value);
  void setDataProvider(DataProvider provider);

  // ===== LOG PROVIDER =====
  typedef void (*LogProvider)(const char* message);
  void setLogProvider(LogProvider provider);

 private:
  ScriptState _active[MAX_ACTIVE_SCRIPTS];
  const char* _queueScript[QUEUE_SIZE];
  uint8_t _queueId[QUEUE_SIZE];
  uint16_t _queueLen[QUEUE_SIZE];
  uint8_t _queueHead, _queueTail, _queueCount;
  ScriptConflict _defaultStrategy;

  PortState _ports[MAX_PORTS];
  uint8_t _portsCount;

  DataSource _dataSources[MAX_DATA_SOURCES];
  uint8_t _dataSourcesCount;

  DataProvider _dataProvider = nullptr;
  LogProvider _logProvider = nullptr;

  int findById(uint8_t id) const;
  bool addToQueue(uint8_t id, const char* script, uint16_t len);
  void activateSlot(int idx, uint8_t id, const char* script, uint16_t len);

  void executeToken(const char* token, ScriptState& s);
  void handleToggle(uint8_t gpio, uint16_t toggleValue);
  void setOutput(uint8_t gpio, uint16_t value);

  void startFade(ScriptState& s, uint8_t gpio, uint16_t target, uint16_t duration, bool isToggle, uint16_t toggleValue);
  void updateFade(ScriptState& s, uint32_t now);

  bool getPortValue(const char* id, uint32_t& value);
  bool getDataSourceValue(const char* id, uint32_t& value);
  DataSource* findDataSource(const char* id);

  PortState* findPort(uint8_t gpio);
  const PortState* findPort(uint8_t gpio) const;

  bool isLoopStart(const char* token);
  bool isLoopEnd(const char* token);
  uint8_t parseLoopCount(const char* token);
  bool isInfiniteLoop(const char* token);

  bool isIfStart(const char* token);
  bool isElse(const char* token);
  bool isEnd(const char* token);
  bool parseCondition(const char* token, ScriptState& s);

  bool isWaitStart(const char* token);
  bool handleWait(const char* token, ScriptState& s, uint32_t now);
};

#endif