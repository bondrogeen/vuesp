#include "./BytecodeVM.h"

#include <math.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "./BytecodeInstruction.h"
#include "./Opcode.h"

namespace vuesp {
namespace bytecode {

static_assert(kStackSize <= 64, "stack allocator uses a 64-bit mask");

namespace {

const char* vmErrorName(VmError error) {
  switch (error) {
    case VmError::NONE:
      return "none";
    case VmError::PARSE:
      return "parse error";
    case VmError::VERIFY:
      return "verify error";
    case VmError::OPCODE:
      return "invalid opcode";
    case VmError::BOUNDS:
      return "out of bounds";
    case VmError::STACK:
      return "stack error";
    case VmError::TYPE:
      return "type error";
    case VmError::VAR:
      return "variable error";
    case VmError::CONST:
      return "constant error";
    case VmError::DATA:
      return "data error";
    case VmError::RUNTIME:
      return "runtime error";
    case VmError::NO_CONTEXT:
      return "no free context";
    case VmError::NO_SCRIPT:
      return "script not found";
    case VmError::NO_FUNCTION:
      return "function not found";
    case VmError::ARGS:
      return "argument error";
    case VmError::NOT_READY:
      return "not ready";
    default:
      return "unknown";
  }
}

DataKind toDataKind(uint8_t kind) {
  switch (kind) {
    case 0:
      return KIND_INT;
    case 1:
      return KIND_UINT;
    case 2:
      return KIND_FLOAT;
    default:
      return KIND_STRING;
  }
}

float rawToFloat(uint32_t raw) {
  union {
    uint32_t u;
    float f;
  } value;
  value.u = raw;
  return value.f;
}

bool resetsScratch(Opcode op) {
  switch (op) {
    case Opcode::STORE_VAR:
    case Opcode::STORE_DATA:
    case Opcode::ARRAY_SET:
    case Opcode::SET:
    case Opcode::LOG:
    case Opcode::WAIT:
    case Opcode::CALL_SCRIPT:
    case Opcode::RETURN:
    case Opcode::HALT:
      return true;
    default:
      return false;
  }
}

int compareStringsFull(const Value& left, const Value& right) {
  const uint16_t leftLen = left.stringVal.len;
  const uint16_t rightLen = right.stringVal.len;
  const uint16_t minLen = leftLen < rightLen ? leftLen : rightLen;
  const int cmp = memcmp(left.stringVal.data, right.stringVal.data, minLen);
  if (cmp != 0) return cmp;
  if (leftLen == rightLen) return 0;
  return leftLen < rightLen ? -1 : 1;
}

}  // namespace

BytecodeVM::BytecodeVM()
    : _stackUsed(0),
      _scratchUsed(0),
      _nextContext(0),
      _dataProvider(nullptr),
      _logProvider(nullptr),
      _portProvider(nullptr),
      _stateChangeProvider(nullptr),
      _completeCallback(nullptr),
      _lastError(VmError::NONE),
      _lastErrorScript(0),
      _lastErrorPc(0) {
  memset(&_caps, 0, sizeof(_caps));
  reset();
}

void BytecodeVM::reset() {
  memset(_scripts, 0, sizeof(_scripts));
  memset(_externals, 0, sizeof(_externals));
  memset(_contexts, 0, sizeof(_contexts));
  memset(&_ctx, 0, sizeof(_ctx));
  memset(_callParams, 0, sizeof(_callParams));
  memset(_stack, 0, sizeof(_stack));
  memset(_scratch, 0, sizeof(_scratch));
  _stackUsed = 0;
  _scratchUsed = 0;
  _nextContext = 0;
  _lastError = VmError::NONE;
  _lastErrorScript = 0;
  _lastErrorPc = 0;
}

void BytecodeVM::setCapabilities(const Capabilities& caps) { _caps = caps; }

void BytecodeVM::setDataProvider(DataProvider provider) { _dataProvider = provider; }
void BytecodeVM::setLogProvider(LogProvider provider) { _logProvider = provider; }
void BytecodeVM::setPortProvider(PortProvider provider) { _portProvider = provider; }
void BytecodeVM::setStateChangeProvider(StateChangeProvider provider) {
  _stateChangeProvider = provider;
}
void BytecodeVM::setCompleteCallback(ScriptCompleteCallback callback) {
  _completeCallback = callback;
}

void BytecodeVM::setError(VmError error, uint8_t scriptId, uint16_t pc) {
  _lastError = error;
  _lastErrorScript = scriptId;
  _lastErrorPc = pc;
  char buffer[64];
  snprintf(buffer, sizeof(buffer), "ERR[%u:%u] %s", (unsigned)scriptId, (unsigned)pc,
           vmErrorName(error));
  logError(buffer);
}

void BytecodeVM::logError(const char* message) {
  if (_logProvider && message) _logProvider(message);
}

int8_t BytecodeVM::findScriptIndex(uint8_t id) const {
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    if (_scripts[i].used && _scripts[i].id == id) return (int8_t)i;
  }
  return -1;
}

int8_t BytecodeVM::findFreeContext() const {
  for (uint8_t i = 0; i < kMaxContexts; i++) {
    if (!_contexts[i].active) return (int8_t)i;
  }
  return -1;
}

int8_t BytecodeVM::allocateStack(uint16_t size) {
  if (size == 0) return 0;
  if (size > kStackSize) return -1;
  if (size == kStackSize) {
    if (_stackUsed != 0) return -1;
    _stackUsed = ~0ull;
    return 0;
  }
  const uint64_t mask = (1ull << size) - 1ull;
  for (uint16_t base = 0; base + size <= kStackSize; base++) {
    const uint64_t shifted = mask << base;
    if ((_stackUsed & shifted) == 0) {
      _stackUsed |= shifted;
      return (int8_t)base;
    }
  }
  return -1;
}

void BytecodeVM::releaseStack(uint16_t base, uint16_t size) {
  if (size == 0) return;
  if (size >= kStackSize) {
    _stackUsed = 0;
    return;
  }
  const uint64_t mask = ((1ull << size) - 1ull) << base;
  _stackUsed &= ~mask;
}

void BytecodeVM::releaseContext(Context& ctx) {
  if (!ctx.active) return;
  const uint8_t scriptId = ctx.scriptIndex < MAX_SCRIPTS && _scripts[ctx.scriptIndex].used
                               ? _scripts[ctx.scriptIndex].id
                               : 0;
  const bool wasHandler = ctx.handler;
  const uint8_t contextIndex = (uint8_t)(&ctx - &_contexts[0]);
  releaseStack(ctx.stackBase, ctx.stackSize);
  ctx.active = false;
  ctx.waiting = false;
  ctx.handler = false;
  ctx.sp = 0;
  if (!wasHandler && _completeCallback) {
    _completeCallback(contextIndex, scriptId);
  }
}

bool BytecodeVM::startContext(int8_t scriptIndex, uint16_t entry, int8_t handlerIndex,
                              int8_t& contextIndex) {
  if (scriptIndex < 0 || scriptIndex >= MAX_SCRIPTS || !_scripts[scriptIndex].used) {
    setError(VmError::NO_SCRIPT, 0, entry);
    return false;
  }
  const int8_t freeContext = findFreeContext();
  if (freeContext < 0) {
    setError(VmError::NO_CONTEXT, _scripts[scriptIndex].id, entry);
    return false;
  }
  const uint16_t stackSize = _scripts[scriptIndex].view.header.stackMax;
  const int8_t stackBase = allocateStack(stackSize);
  if (stackBase < 0) {
    setError(VmError::NO_CONTEXT, _scripts[scriptIndex].id, entry);
    return false;
  }
  Context& ctx = _contexts[freeContext];
  ctx.active = true;
  ctx.waiting = false;
  ctx.handler = handlerIndex >= 0;
  ctx.scriptIndex = (uint8_t)scriptIndex;
  ctx.handlerIndex = handlerIndex >= 0 ? (uint8_t)handlerIndex : 0;
  ctx.pc = entry;
  ctx.stackBase = (uint16_t)stackBase;
  ctx.stackSize = stackSize;
  ctx.sp = 0;
  ctx.waitUntil = 0;
  contextIndex = freeContext;
  return true;
}

bool BytecodeVM::registerFunction(uint8_t id, ExternalFunction func, void* userData) {
  if (!func) return false;
  for (uint8_t i = 0; i < MAX_EXTERNAL_FUNCTIONS; i++) {
    if (_externals[i].used && _externals[i].id == id) {
      _externals[i].func = func;
      _externals[i].userData = userData;
      return true;
    }
  }
  for (uint8_t i = 0; i < MAX_EXTERNAL_FUNCTIONS; i++) {
    if (!_externals[i].used) {
      _externals[i].used = true;
      _externals[i].id = id;
      _externals[i].func = func;
      _externals[i].userData = userData;
      return true;
    }
  }
  return false;
}

bool BytecodeVM::loadScript(uint8_t id, const uint8_t* data, uint16_t size, bool persistent) {
  if (!data || id > 254) return false;
  ContainerView view;
  ParseError parseError;
  if (!parseContainer(data, size, view, parseError)) {
    setError(VmError::PARSE, id, 0);
    return false;
  }
  VerifyResult verify;
  if (!verifyContainer(view, _caps, verify)) {
    setError(VmError::VERIFY, id, verify.offset);
    return false;
  }

  stopScript(id);
  int8_t index = findScriptIndex(id);
  if (index < 0) {
    for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
      if (!_scripts[i].used) {
        index = (int8_t)i;
        break;
      }
    }
  }
  if (index < 0) {
    setError(VmError::NO_CONTEXT, id, 0);
    return false;
  }
  ScriptEntry& entry = _scripts[index];
  entry.used = true;
  entry.persistent = persistent;
  entry.id = id;
  entry.data = data;
  entry.size = size;
  entry.view = view;
  return true;
}

bool BytecodeVM::removeScript(uint8_t id) {
  const int8_t index = findScriptIndex(id);
  if (index < 0) return false;
  stopScript(id);
  _scripts[index].used = false;
  return true;
}

int8_t BytecodeVM::runScript(uint8_t id) {
  const int8_t index = findScriptIndex(id);
  if (index < 0) {
    setError(VmError::NO_SCRIPT, id, 0);
    return -1;
  }
  stopScript(id);
  int8_t contextIndex = -1;
  if (!startContext(index, _scripts[index].view.header.entryOffset, -1, contextIndex)) {
    return -1;
  }
  return contextIndex;
}

bool BytecodeVM::stopScript(uint8_t id) {
  bool stopped = false;
  for (uint8_t i = 0; i < kMaxContexts; i++) {
    Context& ctx = _contexts[i];
    if (!ctx.active) continue;
    if (ctx.scriptIndex < MAX_SCRIPTS && _scripts[ctx.scriptIndex].used &&
        _scripts[ctx.scriptIndex].id == id) {
      releaseContext(ctx);
      stopped = true;
    }
  }
  return stopped;
}

void BytecodeVM::stopAll() {
  for (uint8_t i = 0; i < kMaxContexts; i++) {
    if (_contexts[i].active) releaseContext(_contexts[i]);
  }
}

bool BytecodeVM::isBusy() const {
  for (uint8_t i = 0; i < kMaxContexts; i++) {
    if (_contexts[i].active) return true;
  }
  return false;
}

uint8_t BytecodeVM::activeContexts() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < kMaxContexts; i++) {
    if (_contexts[i].active) count++;
  }
  return count;
}

uint8_t BytecodeVM::scriptCount() const {
  uint8_t count = 0;
  for (uint8_t i = 0; i < MAX_SCRIPTS; i++) {
    if (_scripts[i].used) count++;
  }
  return count;
}

uint32_t BytecodeVM::getUintVar(uint8_t index) const {
  return index < MAX_UINT_VARS ? _ctx.uintVars[index] : 0;
}

int32_t BytecodeVM::getIntVar(uint8_t index) const {
  return index < MAX_INT_VARS ? _ctx.intVars[index] : 0;
}

float BytecodeVM::getFloatVar(uint8_t index) const {
  return index < MAX_FLOAT_VARS ? (float)_ctx.floatVars[index] : 0.0f;
}

const char* BytecodeVM::getStringVar(uint8_t index) const {
  return index < MAX_STRING_VARS ? _ctx.stringVars[index] : "";
}

uint8_t BytecodeVM::getArrayLen(uint8_t index) const {
  return index < MAX_ARRAY_VARS ? _ctx.arrayLen[index] : 0;
}

void BytecodeVM::update(uint32_t now) {
  if (_caps.vmVersion == 0) return;
  const uint32_t updateStart = micros();
  const uint16_t updateBudget = _caps.updateBudgetUs ? _caps.updateBudgetUs : 3000;
  const uint16_t perContextBudget = _caps.instrPerTick ? _caps.instrPerTick : 32;
  const uint16_t perContextUs = _caps.usPerTick ? _caps.usPerTick : 300;

  for (uint8_t step = 0; step < kMaxContexts; step++) {
    const uint8_t index = (uint8_t)((_nextContext + step) % kMaxContexts);
    Context& ctx = _contexts[index];
    if (!ctx.active) continue;
    if (ctx.waiting) {
      if ((int32_t)(now - ctx.waitUntil) < 0) continue;
      ctx.waiting = false;
    }

    resetScratch();
    const uint32_t contextStart = micros();
    uint16_t budget = perContextBudget;
    while (budget-- > 0 && ctx.active && !ctx.waiting) {
      if (perContextUs && (uint16_t)(micros() - contextStart) >= perContextUs) break;
      if (!execute(ctx, now)) break;
    }
    if (updateBudget && (uint16_t)(micros() - updateStart) >= updateBudget) break;
  }
  _nextContext = (uint8_t)((_nextContext + 1) % kMaxContexts);
}

bool BytecodeVM::pushValue(Context& ctx, const Value& value) {
  if (ctx.sp >= ctx.stackSize) {
    setError(VmError::STACK, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }
  _stack[ctx.stackBase + ctx.sp] = value;
  ctx.sp++;
  return true;
}

bool BytecodeVM::popValue(Context& ctx, Value& value) {
  if (ctx.sp == 0) {
    setError(VmError::STACK, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }
  ctx.sp--;
  value = _stack[ctx.stackBase + ctx.sp];
  return true;
}

Value BytecodeVM::makeInt(int32_t value) const {
  Value result;
  result.type = VAL_INT;
  result.intVal = value;
  return result;
}

Value BytecodeVM::makeUint(uint32_t value) const {
  Value result;
  result.type = VAL_UINT;
  result.uintVal = value;
  return result;
}

Value BytecodeVM::makeFloat(float value) const {
  Value result;
  result.type = VAL_FLOAT;
  result.floatVal = value;
  return result;
}

Value BytecodeVM::makeString(const char* data, uint16_t len) const {
  Value result;
  result.type = VAL_STRING;
  result.stringVal.data = data;
  result.stringVal.len = (uint8_t)(len > 255 ? 255 : len);
  return result;
}

Value BytecodeVM::makeArray(const uint8_t* data, uint8_t len) const {
  Value result;
  result.type = VAL_ARRAY;
  result.arrayVal.data = data;
  result.arrayVal.len = len;
  return result;
}

Value BytecodeVM::makeNone() const {
  Value result;
  result.type = VAL_NONE;
  result.intVal = 0;
  return result;
}

bool BytecodeVM::valueToBool(const Value& value) const {
  switch (value.type) {
    case VAL_INT:
      return value.intVal != 0;
    case VAL_UINT:
      return value.uintVal != 0;
    case VAL_FLOAT:
      return value.floatVal != 0.0f;
    case VAL_STRING:
      return value.stringVal.len > 0;
    case VAL_ARRAY:
      return value.arrayVal.len > 0;
    default:
      return false;
  }
}

int32_t BytecodeVM::valueToInt(const Value& value) const {
  switch (value.type) {
    case VAL_INT:
      return value.intVal;
    case VAL_UINT:
      return (int32_t)value.uintVal;
    case VAL_FLOAT:
      return (int32_t)value.floatVal;
    case VAL_STRING:
      return value.stringVal.len ? (int32_t)(uint8_t)value.stringVal.data[0] : 0;
    case VAL_ARRAY:
      return value.arrayVal.len;
    default:
      return 0;
  }
}

uint32_t BytecodeVM::valueToUint(const Value& value) const {
  return (uint32_t)valueToInt(value);
}

float BytecodeVM::valueToFloat(const Value& value) const {
  switch (value.type) {
    case VAL_FLOAT:
      return value.floatVal;
    case VAL_INT:
      return (float)value.intVal;
    case VAL_UINT:
      return (float)value.uintVal;
    default:
      return (float)valueToInt(value);
  }
}

void BytecodeVM::resetScratch() { _scratchUsed = 0; }

char* BytecodeVM::allocScratch(uint16_t len) {
  if ((uint32_t)_scratchUsed + len > kScratchBytes) return nullptr;
  char* result = _scratch + _scratchUsed;
  _scratchUsed = (uint16_t)(_scratchUsed + len);
  return result;
}

bool BytecodeVM::valueToString(const Value& value, const char*& data, uint16_t& len) {
  switch (value.type) {
    case VAL_STRING:
      data = value.stringVal.data;
      len = value.stringVal.len;
      return true;
    case VAL_INT: {
      char* buffer = allocScratch(16);
      if (!buffer) return false;
      const int written = snprintf(buffer, 16, "%d", value.intVal);
      data = buffer;
      len = (uint16_t)(written < 0 ? 0 : written);
      return true;
    }
    case VAL_UINT: {
      char* buffer = allocScratch(16);
      if (!buffer) return false;
      const int written = snprintf(buffer, 16, "%u", value.uintVal);
      data = buffer;
      len = (uint16_t)(written < 0 ? 0 : written);
      return true;
    }
    case VAL_FLOAT: {
      char* buffer = allocScratch(24);
      if (!buffer) return false;
      const int written = snprintf(buffer, 24, "%.2f", value.floatVal);
      data = buffer;
      len = (uint16_t)(written < 0 ? 0 : written);
      return true;
    }
    case VAL_ARRAY: {
      const uint16_t needed = (uint16_t)(value.arrayVal.len * 4 + 3);
      char* buffer = allocScratch(needed);
      if (!buffer) return false;
      uint16_t pos = 0;
      buffer[pos++] = '[';
      for (uint8_t i = 0; i < value.arrayVal.len; i++) {
        if (i > 0) {
          buffer[pos++] = ',';
          buffer[pos++] = ' ';
        }
        const int written = snprintf(buffer + pos, needed - pos, "%u", value.arrayVal.data[i]);
        if (written > 0) pos = (uint16_t)(pos + written);
      }
      buffer[pos++] = ']';
      buffer[pos] = '\0';
      data = buffer;
      len = pos;
      return true;
    }
    default:
      data = "";
      len = 0;
      return true;
  }
}

bool BytecodeVM::isWritableArray(const Value& value) const {
  if (value.type != VAL_ARRAY) return false;
  for (uint8_t i = 0; i < MAX_ARRAY_VARS; i++) {
    if (value.arrayVal.data == _ctx.arrayVars[i]) return true;
  }
  return false;
}

bool BytecodeVM::readVar(Context& ctx, uint8_t varid, Value& out) {
  const uint8_t type = (uint8_t)((varid >> 4) & 0x0F);
  const uint8_t index = (uint8_t)(varid & 0x0F);
  switch (type) {
    case 0:
      if (index >= MAX_UINT_VARS) return false;
      out = makeUint(_ctx.uintVars[index]);
      return true;
    case 1:
      if (index >= MAX_INT_VARS) return false;
      out = makeInt(_ctx.intVars[index]);
      return true;
    case 2:
      if (index >= MAX_FLOAT_VARS) return false;
      out = makeFloat((float)_ctx.floatVars[index]);
      return true;
    case 3:
      if (index >= MAX_STRING_VARS) return false;
      out = makeString(_ctx.stringVars[index], (uint16_t)strlen(_ctx.stringVars[index]));
      return true;
    case 4:
      if (index >= MAX_ARRAY_VARS) return false;
      out = makeArray(_ctx.arrayVars[index], _ctx.arrayLen[index]);
      return true;
    case 5: {
      uint16_t value = 0;
      if (_portProvider) _portProvider(index, PORT_READ, value);
      out = makeUint(value);
      return true;
    }
    case 6:
      if (index >= MAX_EVENT_PARAMS) return false;
      out = makeInt(_ctx.eventParams[index]);
      return true;
    case 7:
      if (index >= MAX_CALL_PARAMS) return false;
      out = makeInt(_callParams[index]);
      return true;
    default:
      setError(VmError::VAR, _scripts[ctx.scriptIndex].id, ctx.pc);
      return false;
  }
}

bool BytecodeVM::writeVar(Context& ctx, uint8_t varid, const Value& value) {
  const uint8_t type = (uint8_t)((varid >> 4) & 0x0F);
  const uint8_t index = (uint8_t)(varid & 0x0F);
  switch (type) {
    case 0:
      if (index >= MAX_UINT_VARS) return false;
      _ctx.uintVars[index] = valueToUint(value);
      return true;
    case 1:
      if (index >= MAX_INT_VARS) return false;
      _ctx.intVars[index] = valueToInt(value);
      return true;
    case 2:
      if (index >= MAX_FLOAT_VARS) return false;
      _ctx.floatVars[index] = valueToFloat(value);
      return true;
    case 3: {
      if (index >= MAX_STRING_VARS) return false;
      if (value.type == VAL_STRING) {
        uint16_t len = value.stringVal.len;
        if (len >= MAX_STRING_LEN) len = MAX_STRING_LEN - 1;
        memcpy(_ctx.stringVars[index], value.stringVal.data, len);
        _ctx.stringVars[index][len] = '\0';
      } else {
        _ctx.stringVars[index][0] = (char)valueToInt(value);
        _ctx.stringVars[index][1] = '\0';
      }
      return true;
    }
    case 4: {
      if (index >= MAX_ARRAY_VARS) return false;
      if (value.type != VAL_ARRAY) {
        setError(VmError::TYPE, _scripts[ctx.scriptIndex].id, ctx.pc);
        return false;
      }
      uint16_t len = value.arrayVal.len;
      if (len > MAX_ARRAY_SIZE) len = MAX_ARRAY_SIZE;
      memcpy(_ctx.arrayVars[index], value.arrayVal.data, len);
      _ctx.arrayLen[index] = (uint8_t)len;
      return true;
    }
    case 5: {
      uint16_t portValue = (uint16_t)valueToUint(value);
      if (portValue > MAX_PWM_VALUE) portValue = MAX_PWM_VALUE;
      if (_portProvider) _portProvider(index, PORT_WRITE, portValue);
      if (_stateChangeProvider) _stateChangeProvider(index, portValue);
      return true;
    }
    default:
      setError(VmError::VAR, _scripts[ctx.scriptIndex].id, ctx.pc);
      return false;
  }
}

bool BytecodeVM::readData(const ContainerView& view, uint16_t nameConst, uint8_t kind,
                          Value& out) {
  ConstantRef ref;
  if (!getConstant(view, nameConst, ref) || ref.len >= 64) return false;
  char name[64];
  memcpy(name, ref.data, ref.len);
  name[ref.len] = '\0';
  if (!_dataProvider) return false;

  DataValue dataValue;
  memset(&dataValue, 0, sizeof(dataValue));
  const DataKind dataKind = toDataKind(kind);
  if (!_dataProvider(name, dataKind, dataValue, false)) return false;

  switch (dataKind) {
    case KIND_INT:
      out = makeInt(dataValue.intVal);
      return true;
    case KIND_UINT:
      out = makeUint(dataValue.uintVal);
      return true;
    case KIND_FLOAT:
      out = makeFloat(dataValue.floatVal);
      return true;
    case KIND_STRING: {
      const uint16_t len = dataValue.stringVal.len;
      char* buffer = allocScratch((uint16_t)(len + 1));
      if (!buffer) return false;
      if (len > 0 && dataValue.stringVal.data) {
        memcpy(buffer, dataValue.stringVal.data, len);
      }
      buffer[len] = '\0';
      out = makeString(buffer, len);
      return true;
    }
    default:
      return false;
  }
}

bool BytecodeVM::writeData(const ContainerView& view, uint16_t nameConst, const Value& value) {
  ConstantRef ref;
  if (!getConstant(view, nameConst, ref) || ref.len >= 64) return false;
  char name[64];
  memcpy(name, ref.data, ref.len);
  name[ref.len] = '\0';
  if (!_dataProvider) return false;

  DataValue dataValue;
  memset(&dataValue, 0, sizeof(dataValue));
  DataKind kind = KIND_INT;
  switch (value.type) {
    case VAL_INT:
      kind = KIND_INT;
      dataValue.intVal = value.intVal;
      break;
    case VAL_UINT:
      kind = KIND_UINT;
      dataValue.uintVal = value.uintVal;
      break;
    case VAL_FLOAT:
      kind = KIND_FLOAT;
      dataValue.floatVal = value.floatVal;
      break;
    case VAL_STRING:
      kind = KIND_STRING;
      dataValue.stringVal.data = (uint8_t*)value.stringVal.data;
      dataValue.stringVal.len = value.stringVal.len;
      break;
    default:
      return false;
  }
  return _dataProvider(name, kind, dataValue, true);
}

bool BytecodeVM::binaryOp(Context& ctx, Opcode op) {
  Value right;
  Value left;
  if (!popValue(ctx, right) || !popValue(ctx, left)) return false;
  const uint8_t scriptId = _scripts[ctx.scriptIndex].id;

  if (op == Opcode::AND) {
    return pushValue(ctx, makeInt(valueToBool(left) && valueToBool(right) ? 1 : 0));
  }
  if (op == Opcode::OR) {
    return pushValue(ctx, makeInt(valueToBool(left) || valueToBool(right) ? 1 : 0));
  }

  if (op == Opcode::ADD && (left.type == VAL_STRING || right.type == VAL_STRING)) {
    const char* leftData = nullptr;
    const char* rightData = nullptr;
    uint16_t leftLen = 0;
    uint16_t rightLen = 0;
    if (!valueToString(left, leftData, leftLen) || !valueToString(right, rightData, rightLen)) {
      setError(VmError::RUNTIME, scriptId, ctx.pc);
      return false;
    }
    char* buffer = allocScratch((uint16_t)(leftLen + rightLen + 1));
    if (!buffer) {
      setError(VmError::RUNTIME, scriptId, ctx.pc);
      return false;
    }
    memcpy(buffer, leftData, leftLen);
    memcpy(buffer + leftLen, rightData, rightLen);
    buffer[leftLen + rightLen] = '\0';
    return pushValue(ctx, makeString(buffer, (uint16_t)(leftLen + rightLen)));
  }

  const bool comparison =
      op == Opcode::EQ || op == Opcode::NE || op == Opcode::LT || op == Opcode::LE ||
      op == Opcode::GT || op == Opcode::GE;
  if (comparison && (left.type == VAL_STRING || right.type == VAL_STRING)) {
    const char* leftData = nullptr;
    const char* rightData = nullptr;
    uint16_t leftLen = 0;
    uint16_t rightLen = 0;
    if (!valueToString(left, leftData, leftLen) || !valueToString(right, rightData, rightLen)) {
      setError(VmError::RUNTIME, scriptId, ctx.pc);
      return false;
    }
    const Value leftString = makeString(leftData, leftLen);
    const Value rightString = makeString(rightData, rightLen);
    const int cmp = compareStringsFull(leftString, rightString);
    bool result = false;
    if (op == Opcode::EQ) result = cmp == 0;
    else if (op == Opcode::NE) result = cmp != 0;
    else if (op == Opcode::LT) result = cmp < 0;
    else if (op == Opcode::LE) result = cmp <= 0;
    else if (op == Opcode::GT) result = cmp > 0;
    else result = cmp >= 0;
    return pushValue(ctx, makeInt(result ? 1 : 0));
  }

  if (left.type == VAL_FLOAT || right.type == VAL_FLOAT) {
    const float l = valueToFloat(left);
    const float r = valueToFloat(right);
    switch (op) {
      case Opcode::ADD:
        return pushValue(ctx, makeFloat(l + r));
      case Opcode::SUB:
        return pushValue(ctx, makeFloat(l - r));
      case Opcode::MUL:
        return pushValue(ctx, makeFloat(l * r));
      case Opcode::DIV:
        if (r == 0.0f) {
          setError(VmError::RUNTIME, scriptId, ctx.pc);
          return false;
        }
        return pushValue(ctx, makeFloat(l / r));
      case Opcode::MOD:
        if (r == 0.0f) {
          setError(VmError::RUNTIME, scriptId, ctx.pc);
          return false;
        }
        return pushValue(ctx, makeFloat(fmodf(l, r)));
      case Opcode::EQ:
        return pushValue(ctx, makeInt(l == r ? 1 : 0));
      case Opcode::NE:
        return pushValue(ctx, makeInt(l != r ? 1 : 0));
      case Opcode::LT:
        return pushValue(ctx, makeInt(l < r ? 1 : 0));
      case Opcode::LE:
        return pushValue(ctx, makeInt(l <= r ? 1 : 0));
      case Opcode::GT:
        return pushValue(ctx, makeInt(l > r ? 1 : 0));
      case Opcode::GE:
        return pushValue(ctx, makeInt(l >= r ? 1 : 0));
      default:
        break;
    }
  }

  const bool useUint = left.type == VAL_UINT && right.type == VAL_UINT;
  if (useUint) {
    const uint32_t l = left.uintVal;
    const uint32_t r = right.uintVal;
    switch (op) {
      case Opcode::ADD:
        return pushValue(ctx, makeUint(l + r));
      case Opcode::SUB:
        return pushValue(ctx, makeUint(l - r));
      case Opcode::MUL:
        return pushValue(ctx, makeUint(l * r));
      case Opcode::DIV:
        if (r == 0) {
          setError(VmError::RUNTIME, scriptId, ctx.pc);
          return false;
        }
        return pushValue(ctx, makeUint(l / r));
      case Opcode::MOD:
        if (r == 0) {
          setError(VmError::RUNTIME, scriptId, ctx.pc);
          return false;
        }
        return pushValue(ctx, makeUint(l % r));
      case Opcode::BIT_AND:
        return pushValue(ctx, makeUint(l & r));
      case Opcode::BIT_OR:
        return pushValue(ctx, makeUint(l | r));
      case Opcode::BIT_XOR:
        return pushValue(ctx, makeUint(l ^ r));
      case Opcode::SHL:
        return pushValue(ctx, makeUint(l << (r & 31u)));
      case Opcode::SHR:
        return pushValue(ctx, makeUint(l >> (r & 31u)));
      case Opcode::EQ:
        return pushValue(ctx, makeInt(l == r ? 1 : 0));
      case Opcode::NE:
        return pushValue(ctx, makeInt(l != r ? 1 : 0));
      case Opcode::LT:
        return pushValue(ctx, makeInt(l < r ? 1 : 0));
      case Opcode::LE:
        return pushValue(ctx, makeInt(l <= r ? 1 : 0));
      case Opcode::GT:
        return pushValue(ctx, makeInt(l > r ? 1 : 0));
      case Opcode::GE:
        return pushValue(ctx, makeInt(l >= r ? 1 : 0));
      default:
        setError(VmError::TYPE, scriptId, ctx.pc);
        return false;
    }
  } else {
    const int32_t l = valueToInt(left);
    const int32_t r = valueToInt(right);
    switch (op) {
      case Opcode::ADD:
        return pushValue(ctx, makeInt(l + r));
      case Opcode::SUB:
        return pushValue(ctx, makeInt(l - r));
      case Opcode::MUL:
        return pushValue(ctx, makeInt(l * r));
      case Opcode::DIV:
        if (r == 0) {
          setError(VmError::RUNTIME, scriptId, ctx.pc);
          return false;
        }
        return pushValue(ctx, makeInt(l / r));
      case Opcode::MOD:
        if (r == 0) {
          setError(VmError::RUNTIME, scriptId, ctx.pc);
          return false;
        }
        return pushValue(ctx, makeInt(l % r));
      case Opcode::BIT_AND:
        return pushValue(ctx, makeInt(l & r));
      case Opcode::BIT_OR:
        return pushValue(ctx, makeInt(l | r));
      case Opcode::BIT_XOR:
        return pushValue(ctx, makeInt(l ^ r));
      case Opcode::SHL:
        return pushValue(ctx, makeInt(l << (r & 31)));
      case Opcode::SHR:
        return pushValue(ctx, makeInt(l >> (r & 31)));
      case Opcode::EQ:
        return pushValue(ctx, makeInt(l == r ? 1 : 0));
      case Opcode::NE:
        return pushValue(ctx, makeInt(l != r ? 1 : 0));
      case Opcode::LT:
        return pushValue(ctx, makeInt(l < r ? 1 : 0));
      case Opcode::LE:
        return pushValue(ctx, makeInt(l <= r ? 1 : 0));
      case Opcode::GT:
        return pushValue(ctx, makeInt(l > r ? 1 : 0));
      case Opcode::GE:
        return pushValue(ctx, makeInt(l >= r ? 1 : 0));
      default:
        setError(VmError::TYPE, scriptId, ctx.pc);
        return false;
    }
  }
}

bool BytecodeVM::unaryOp(Context& ctx, Opcode op) {
  Value value;
  if (!popValue(ctx, value)) return false;
  const uint8_t scriptId = _scripts[ctx.scriptIndex].id;
  switch (op) {
    case Opcode::NEG:
      if (value.type == VAL_FLOAT) return pushValue(ctx, makeFloat(-value.floatVal));
      return pushValue(ctx, makeInt(-valueToInt(value)));
    case Opcode::NOT:
      return pushValue(ctx, makeInt(valueToBool(value) ? 0 : 1));
    case Opcode::BIT_NOT:
      if (value.type == VAL_UINT) return pushValue(ctx, makeUint(~value.uintVal));
      return pushValue(ctx, makeInt(~valueToInt(value)));
    default:
      setError(VmError::TYPE, scriptId, ctx.pc);
      return false;
  }
}

bool BytecodeVM::callExternal(Context& ctx, uint8_t functionId, uint8_t argc) {
  if (argc > MAX_FUNCTION_PARAMS) {
    setError(VmError::ARGS, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }
  Value params[MAX_FUNCTION_PARAMS];
  for (int i = argc - 1; i >= 0; i--) {
    if (!popValue(ctx, params[i])) return false;
  }

  ExternalEntry* entry = nullptr;
  for (uint8_t i = 0; i < MAX_EXTERNAL_FUNCTIONS; i++) {
    if (_externals[i].used && _externals[i].id == functionId) {
      entry = &_externals[i];
      break;
    }
  }
  if (!entry) {
    setError(VmError::NO_FUNCTION, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }

  const FunctionSpec* spec = nullptr;
  for (uint8_t i = 0; i < _caps.functionCount; i++) {
    if (_caps.functions[i].id == functionId) {
      spec = &_caps.functions[i];
      break;
    }
  }
  if (!spec) {
    setError(VmError::NO_FUNCTION, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }

  Value result = makeNone();
  if (!entry->func(argc, params, result, entry->userData)) {
    setError(VmError::RUNTIME, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }

  if (spec->ret == VALUE_NONE) {
    resetScratch();
    return true;
  }
  if (result.type == VAL_NONE) {
    switch (spec->ret) {
      case VALUE_INT:
        result = makeInt(0);
        break;
      case VALUE_UINT:
        result = makeUint(0);
        break;
      case VALUE_FLOAT:
        result = makeFloat(0.0f);
        break;
      case VALUE_STRING:
        result = makeString("", 0);
        break;
      default:
        result = makeInt(0);
        break;
    }
  }
  return pushValue(ctx, result);
}

bool BytecodeVM::callScript(Context& ctx, uint8_t scriptId, uint8_t argc) {
  if (argc > MAX_CALL_PARAMS) {
    setError(VmError::ARGS, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }
  if (scriptId == _scripts[ctx.scriptIndex].id) {
    setError(VmError::RUNTIME, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }

  int32_t args[MAX_CALL_PARAMS];
  for (int i = argc - 1; i >= 0; i--) {
    Value value;
    if (!popValue(ctx, value)) return false;
    args[i] = valueToInt(value);
  }

  const int8_t target = findScriptIndex(scriptId);
  if (target < 0) {
    setError(VmError::NO_SCRIPT, scriptId, ctx.pc);
    return false;
  }
  stopScript(scriptId);
  for (uint8_t i = 0; i < MAX_CALL_PARAMS; i++) {
    _callParams[i] = i < argc ? args[i] : 0;
  }
  int8_t newContext = -1;
  return startContext(target, _scripts[target].view.header.entryOffset, -1, newContext);
}

bool BytecodeVM::logValues(Context& ctx, uint8_t argc) {
  if (argc > 8) {
    setError(VmError::ARGS, _scripts[ctx.scriptIndex].id, ctx.pc);
    return false;
  }
  Value values[8];
  for (int i = argc - 1; i >= 0; i--) {
    if (!popValue(ctx, values[i])) return false;
  }

  char buffer[192];
  int prefix = snprintf(buffer, sizeof(buffer), "LOG[%u]: ", (unsigned)_scripts[ctx.scriptIndex].id);
  uint16_t pos = prefix > 0 ? (uint16_t)prefix : 0;
  for (uint8_t i = 0; i < argc; i++) {
    if (i > 0 && pos < sizeof(buffer) - 1) buffer[pos++] = ' ';
    const char* data = nullptr;
    uint16_t len = 0;
    if (!valueToString(values[i], data, len)) return false;
    for (uint16_t j = 0; j < len && pos < sizeof(buffer) - 1; j++) {
      buffer[pos++] = data[j];
    }
  }
  buffer[pos] = '\0';
  if (_logProvider) _logProvider(buffer);
  return true;
}

bool BytecodeVM::wait(Context& ctx, uint32_t ms, uint32_t now) {
  if (ms < 10) ms = 10;
  ctx.waiting = true;
  ctx.waitUntil = now + ms;
  return true;
}

bool BytecodeVM::execute(Context& ctx, uint32_t now) {
  if (ctx.scriptIndex >= MAX_SCRIPTS || !_scripts[ctx.scriptIndex].used) {
    setError(VmError::NO_SCRIPT, 0, ctx.pc);
    releaseContext(ctx);
    return false;
  }
  ScriptEntry& script = _scripts[ctx.scriptIndex];
  const ContainerView& view = script.view;
  DecodedInstruction inst;
  if (!decodeInstruction(view.code, view.header.codeLength, ctx.pc, inst)) {
    setError(VmError::OPCODE, script.id, ctx.pc);
    releaseContext(ctx);
    return false;
  }
  ctx.pc = (uint16_t)(ctx.pc + inst.size);
  const uint8_t scriptId = script.id;

  auto abort = [&]() -> bool {
    releaseContext(ctx);
    return false;
  };

  switch (inst.op) {
    case Opcode::PUSH_I8:
      if (!pushValue(ctx, makeInt((int8_t)inst.operands[0]))) return abort();
      break;
    case Opcode::PUSH_I16:
      if (!pushValue(ctx, makeInt((int16_t)inst.operands[0]))) return abort();
      break;
    case Opcode::PUSH_I32:
      if (!pushValue(ctx, makeInt((int32_t)inst.operands[0]))) return abort();
      break;
    case Opcode::PUSH_U8:
    case Opcode::PUSH_U16:
    case Opcode::PUSH_U32:
      if (!pushValue(ctx, makeUint(inst.operands[0]))) return abort();
      break;
    case Opcode::PUSH_F32:
      if (!pushValue(ctx, makeFloat(rawToFloat(inst.operands[0])))) return abort();
      break;
    case Opcode::PUSH_STR: {
      ConstantRef ref;
      if (!getConstant(view, (uint16_t)inst.operands[0], ref)) {
        setError(VmError::CONST, scriptId, ctx.pc);
        return abort();
      }
      if (!pushValue(ctx, makeString((const char*)ref.data, ref.len))) return abort();
      break;
    }
    case Opcode::PUSH_ARRAY: {
      ConstantRef ref;
      if (!getConstant(view, (uint16_t)inst.operands[0], ref)) {
        setError(VmError::CONST, scriptId, ctx.pc);
        return abort();
      }
      if (!pushValue(ctx, makeArray(ref.data, (uint8_t)ref.len))) return abort();
      break;
    }
    case Opcode::LOAD_VAR: {
      Value value;
      if (!readVar(ctx, (uint8_t)inst.operands[0], value)) return abort();
      if (!pushValue(ctx, value)) return abort();
      break;
    }
    case Opcode::STORE_VAR: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      if (!writeVar(ctx, (uint8_t)inst.operands[0], value)) return abort();
      break;
    }
    case Opcode::LOAD_DATA: {
      Value value;
      if (!readData(view, (uint16_t)inst.operands[0], (uint8_t)inst.operands[1], value)) {
        setError(VmError::DATA, scriptId, ctx.pc);
        return abort();
      }
      if (!pushValue(ctx, value)) return abort();
      break;
    }
    case Opcode::STORE_DATA: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      if (!writeData(view, (uint16_t)inst.operands[0], value)) {
        setError(VmError::DATA, scriptId, ctx.pc);
        return abort();
      }
      break;
    }
    case Opcode::DUP: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      if (!pushValue(ctx, value)) return abort();
      if (!pushValue(ctx, value)) return abort();
      break;
    }
    case Opcode::POP: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      break;
    }
    case Opcode::SWAP: {
      Value a;
      Value b;
      if (!popValue(ctx, b) || !popValue(ctx, a)) return abort();
      if (!pushValue(ctx, b)) return abort();
      if (!pushValue(ctx, a)) return abort();
      break;
    }
    case Opcode::ADD:
    case Opcode::SUB:
    case Opcode::MUL:
    case Opcode::DIV:
    case Opcode::MOD:
    case Opcode::BIT_AND:
    case Opcode::BIT_OR:
    case Opcode::BIT_XOR:
    case Opcode::SHL:
    case Opcode::SHR:
    case Opcode::EQ:
    case Opcode::NE:
    case Opcode::LT:
    case Opcode::LE:
    case Opcode::GT:
    case Opcode::GE:
    case Opcode::AND:
    case Opcode::OR:
      if (!binaryOp(ctx, inst.op)) return abort();
      break;
    case Opcode::NEG:
    case Opcode::NOT:
    case Opcode::BIT_NOT:
      if (!unaryOp(ctx, inst.op)) return abort();
      break;
    case Opcode::JUMP:
      ctx.pc = (uint16_t)inst.operands[0];
      break;
    case Opcode::JUMP_IF_FALSE: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      if (!valueToBool(value)) ctx.pc = (uint16_t)inst.operands[0];
      break;
    }
    case Opcode::JUMP_IF_TRUE: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      if (valueToBool(value)) ctx.pc = (uint16_t)inst.operands[0];
      break;
    }
    case Opcode::WAIT:
      wait(ctx, inst.operands[0], now);
      return false;
    case Opcode::LOG:
      if (!logValues(ctx, (uint8_t)inst.operands[0])) return abort();
      break;
    case Opcode::CALL_SCRIPT:
      if (!callScript(ctx, (uint8_t)inst.operands[0], (uint8_t)inst.operands[1])) {
        return abort();
      }
      break;
    case Opcode::CALL_EXT:
      if (!callExternal(ctx, (uint8_t)inst.operands[0], (uint8_t)inst.operands[1])) {
        return abort();
      }
      break;
    case Opcode::SET:
    case Opcode::ARRAY_SET: {
      Value value;
      Value index;
      Value array;
      if (!popValue(ctx, value) || !popValue(ctx, index) || !popValue(ctx, array)) {
        return abort();
      }
      if (array.type != VAL_ARRAY || !isWritableArray(array)) {
        setError(VmError::TYPE, scriptId, ctx.pc);
        return abort();
      }
      const int32_t idx = valueToInt(index);
      if (idx < 0 || idx >= array.arrayVal.len) {
        setError(VmError::BOUNDS, scriptId, ctx.pc);
        return abort();
      }
      const_cast<uint8_t*>(array.arrayVal.data)[idx] = (uint8_t)valueToInt(value);
      break;
    }
    case Opcode::GET:
    case Opcode::ARRAY_GET: {
      Value index;
      Value array;
      if (!popValue(ctx, index) || !popValue(ctx, array)) return abort();
      if (array.type != VAL_ARRAY) {
        setError(VmError::TYPE, scriptId, ctx.pc);
        return abort();
      }
      const int32_t idx = valueToInt(index);
      if (idx < 0 || idx >= array.arrayVal.len) {
        setError(VmError::BOUNDS, scriptId, ctx.pc);
        return abort();
      }
      if (!pushValue(ctx, makeInt(array.arrayVal.data[idx]))) return abort();
      break;
    }
    case Opcode::LEN:
    case Opcode::ARRAY_LEN: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      uint32_t len = 0;
      if (value.type == VAL_STRING) len = value.stringVal.len;
      else if (value.type == VAL_ARRAY) len = value.arrayVal.len;
      if (!pushValue(ctx, makeUint(len))) return abort();
      break;
    }
    case Opcode::CHR: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      char* buffer = allocScratch(2);
      if (!buffer) {
        setError(VmError::RUNTIME, scriptId, ctx.pc);
        return abort();
      }
      buffer[0] = (char)valueToInt(value);
      buffer[1] = '\0';
      if (!pushValue(ctx, makeString(buffer, 1))) return abort();
      break;
    }
    case Opcode::ORD: {
      Value value;
      if (!popValue(ctx, value)) return abort();
      int32_t result = 0;
      if (value.type == VAL_STRING && value.stringVal.len > 0) {
        result = (uint8_t)value.stringVal.data[0];
      } else if (value.type == VAL_ARRAY && value.arrayVal.len > 0) {
        result = value.arrayVal.data[0];
      } else {
        result = valueToInt(value) & 0xff;
      }
      if (!pushValue(ctx, makeInt(result))) return abort();
      break;
    }
    case Opcode::HALT:
    case Opcode::RETURN:
      releaseContext(ctx);
      return false;
    default:
      setError(VmError::OPCODE, scriptId, ctx.pc);
      return abort();
  }

  if (resetsScratch(inst.op)) resetScratch();
  return ctx.active && !ctx.waiting;
}

bool BytecodeVM::emitToHandler(int8_t scriptIndex, uint8_t handlerIndex, uint8_t paramCount,
                               const int32_t* params) {
  if (scriptIndex < 0 || scriptIndex >= MAX_SCRIPTS || !_scripts[scriptIndex].used) return false;
  HandlerRef handler;
  if (!getHandler(_scripts[scriptIndex].view, handlerIndex, handler)) return false;
  for (uint8_t i = 0; i < MAX_EVENT_PARAMS; i++) {
    _ctx.eventParams[i] = (i < paramCount && params) ? params[i] : 0;
  }
  _ctx.eventParamCount = paramCount > MAX_EVENT_PARAMS ? MAX_EVENT_PARAMS : paramCount;
  int8_t contextIndex = -1;
  return startContext(scriptIndex, handler.entry, (int8_t)handlerIndex, contextIndex);
}

bool BytecodeVM::emitEvent(uint8_t eventId, uint8_t paramCount, const int32_t* params) {
  bool started = false;
  for (uint8_t s = 0; s < MAX_SCRIPTS; s++) {
    if (!_scripts[s].used) continue;
    const ContainerView& view = _scripts[s].view;
    for (uint16_t h = 0; h < view.header.handlerCount; h++) {
      HandlerRef handler;
      if (!getHandler(view, h, handler)) continue;
      if (handler.eventId != eventId) continue;
      if (emitToHandler((int8_t)s, (uint8_t)h, paramCount, params)) {
        started = true;
      } else {
        logError("event dropped: no free context");
      }
    }
  }
  return started;
}

bool BytecodeVM::emitEvent(const char* eventName, uint8_t paramCount, ...) {
  if (!eventName) return false;
  uint8_t eventId = 0;
  bool found = false;
  for (uint8_t i = 0; i < _caps.eventCount; i++) {
    if (_caps.events[i].name && strcmp(_caps.events[i].name, eventName) == 0) {
      eventId = _caps.events[i].id;
      found = true;
      break;
    }
  }
  if (!found) return false;

  int32_t params[MAX_EVENT_PARAMS];
  memset(params, 0, sizeof(params));
  va_list args;
  va_start(args, paramCount);
  for (uint8_t i = 0; i < paramCount && i < MAX_EVENT_PARAMS; i++) {
    params[i] = va_arg(args, int);
  }
  va_end(args);
  return emitEvent(eventId, paramCount, params);
}

}  // namespace bytecode
}  // namespace vuesp
