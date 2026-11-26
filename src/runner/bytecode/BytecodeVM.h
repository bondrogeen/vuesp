#ifndef VUESP_BYTECODE_VM_H
#define VUESP_BYTECODE_VM_H

#include <stdint.h>

#include "../ScriptRunner.h"
#include "./BytecodeCapabilities.h"
#include "./BytecodeFormat.h"
#include "./BytecodeVerifier.h"
#include "./Opcode.h"

namespace vuesp {
namespace bytecode {

enum class VmError : uint8_t {
  NONE = 0,
  PARSE,
  VERIFY,
  OPCODE,
  BOUNDS,
  STACK,
  TYPE,
  VAR,
  CONST,
  DATA,
  RUNTIME,
  NO_CONTEXT,
  NO_SCRIPT,
  NO_FUNCTION,
  ARGS,
  NOT_READY,
  CODE_CRC,
  NO_CACHE,
  RECURSION,
  DEPTH
};

using ScriptResolver = bool (*)(void* userData, const char* name, uint8_t& id,
                                const uint8_t*& data, uint16_t& size,
                                VmError& error);

class BytecodeVM {
 public:
  BytecodeVM();

  void reset();
  void setCapabilities(const Capabilities& caps);
  const Capabilities& capabilities() const { return _caps; }

  void setDataProvider(DataProvider provider);
  void setLogProvider(LogProvider provider);
  void setPortProvider(PortProvider provider);
  void setStateChangeProvider(StateChangeProvider provider);
  void setCompleteCallback(ScriptCompleteCallback callback);
  void setScriptResolver(ScriptResolver resolver, void* userData = nullptr);
  void reportError(VmError error, uint8_t scriptId, uint16_t pc) {
    setError(error, scriptId, pc);
  }

  bool registerFunction(uint8_t id, ExternalFunction func, void* userData = nullptr);

  bool loadScript(uint8_t id, const uint8_t* data, uint16_t size, bool persistent = false);
  bool removeScript(uint8_t id);
  int8_t runScript(uint8_t id);
  bool stopScript(uint8_t id);
  void stopAll();
  void update(uint32_t now);
  bool isBusy() const;

  bool emitEvent(uint8_t eventId, uint8_t paramCount, const int32_t* params);
  bool emitEvent(const char* eventName, uint8_t paramCount, ...);

  uint8_t activeContexts() const;
  uint8_t scriptCount() const;
  bool isScriptActive(uint8_t id) const;
  uint32_t getUintVar(uint8_t index) const;
  int32_t getIntVar(uint8_t index) const;
  float getFloatVar(uint8_t index) const;
  const char* getStringVar(uint8_t index) const;
  uint8_t getArrayLen(uint8_t index) const;
  VmError lastError() const { return _lastError; }
  uint8_t lastErrorScript() const { return _lastErrorScript; }
  uint16_t lastErrorPc() const { return _lastErrorPc; }

 private:
  struct ScriptEntry {
    bool used;
    bool persistent;
    uint8_t id;
    const uint8_t* data;
    uint16_t size;
    ContainerView view;
  };

  struct ExternalEntry {
    bool used;
    uint8_t id;
    ExternalFunction func;
    void* userData;
  };

  struct Context {
    bool active;
    bool waiting;
    bool handler;
    uint8_t scriptIndex;
    uint8_t handlerIndex;
    uint16_t pc;
    uint16_t stackBase;
    uint16_t stackSize;
    uint16_t sp;
    uint32_t waitUntil;
    uint8_t ancestors[kMaxCallDepth];
    uint8_t ancestorCount;
    int32_t callParams[MAX_CALL_PARAMS];
  };

  Capabilities _caps;
  ScriptEntry _scripts[MAX_SCRIPTS];
  ExternalEntry _externals[MAX_EXTERNAL_FUNCTIONS];
  Context _contexts[kMaxContexts];
  Value _stack[kStackSize];
  uint64_t _stackUsed;
  ScriptContext _ctx;
  char _scratch[kScratchBytes];
  uint16_t _scratchUsed;
  uint8_t _nextContext;

  DataProvider _dataProvider;
  LogProvider _logProvider;
  PortProvider _portProvider;
  StateChangeProvider _stateChangeProvider;
  ScriptCompleteCallback _completeCallback;
  ScriptResolver _scriptResolver;
  void* _scriptResolverData;

  VmError _lastError;
  uint8_t _lastErrorScript;
  uint16_t _lastErrorPc;

  void setError(VmError error, uint8_t scriptId, uint16_t pc);
  void logError(const char* message);

  int8_t findScriptIndex(uint8_t id) const;
  int8_t findFreeContext() const;
  int8_t allocateStack(uint16_t size);
  void releaseStack(uint16_t base, uint16_t size);
  void releaseContext(Context& ctx);
  bool startContext(int8_t scriptIndex, uint16_t entry, int8_t handlerIndex,
                    int8_t& contextIndex, int8_t parentContext = -1);
  uint8_t callDepth(const Context& ctx) const;
  bool inCallChain(const Context& ctx, uint8_t scriptId) const;

  bool execute(Context& ctx, uint32_t now);
  bool pushValue(Context& ctx, const Value& value);
  bool popValue(Context& ctx, Value& value);

  Value makeInt(int32_t value) const;
  Value makeUint(uint32_t value) const;
  Value makeFloat(float value) const;
  Value makeString(const char* data, uint16_t len) const;
  Value makeArray(const uint8_t* data, uint8_t len) const;
  Value makeNone() const;

  bool valueToBool(const Value& value) const;
  int32_t valueToInt(const Value& value) const;
  uint32_t valueToUint(const Value& value) const;
  float valueToFloat(const Value& value) const;

  void resetScratch();
  char* allocScratch(uint16_t len);
  bool valueToString(const Value& value, const char*& data, uint16_t& len);

  bool isWritableArray(const Value& value) const;
  bool readVar(Context& ctx, uint8_t varid, Value& out);
  bool writeVar(Context& ctx, uint8_t varid, const Value& value);
  bool readData(const ContainerView& view, uint16_t nameConst, uint8_t kind, Value& out);
  bool writeData(const ContainerView& view, uint16_t nameConst, const Value& value);

  bool binaryOp(Context& ctx, Opcode op);
  bool unaryOp(Context& ctx, Opcode op);
  bool callExternal(Context& ctx, uint8_t functionId, uint8_t argc);
  bool callScript(Context& ctx, uint8_t scriptId, uint8_t argc);
  bool callScriptByName(Context& ctx, uint16_t nameConst, uint8_t argc);
  bool launchScript(Context& ctx, uint8_t scriptId, uint8_t argc,
                    const int32_t* args);
  bool logValues(Context& ctx, uint8_t argc);
  bool wait(Context& ctx, uint32_t ms, uint32_t now);
  bool emitToHandler(int8_t scriptIndex, uint8_t handlerIndex, uint8_t paramCount,
                     const int32_t* params);
};

}  // namespace bytecode
}  // namespace vuesp

#endif
