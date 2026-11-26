#include <stdio.h>
#include <string.h>

#include "../runner/bytecode/BytecodeFormat.h"
#include "../runner/bytecode/BytecodeVM.h"
#include "../runner/bytecode/BytecodeVerifier.h"
#include "../runner/bytecode/Opcode.h"
#include "arduino_stub.h"
#include "fixtures/bytecode_vectors.h"

using namespace vuesp::bytecode;

static const FunctionSpec kFunctions[] = {
    {1, "fade", 3, 3, VALUE_INT},
    {2, "udp", 1, 2, VALUE_INT},
    {3, "http", 2, 5, VALUE_INT},
};

static const EventSpec kEvents[] = {
    {1, "btn", 2},
    {2, "btn_c", 3},
    {3, "btn_l", 2},
    {4, "btn_r", 2},
};

static const Capabilities kCaps = {
    1,
    1024,
    64,
    {10, 10, 5, 2, 5, 40, 4, 4},
    32,
    64,
    32,
    300,
    3000,
    kFunctions,
    3,
    kEvents,
    4,
};

static void fixCrc(uint8_t* data, uint16_t size) {
  const uint32_t crc = crc32Container(data, size);
  data[24] = (uint8_t)(crc & 0xff);
  data[25] = (uint8_t)((crc >> 8) & 0xff);
  data[26] = (uint8_t)((crc >> 16) & 0xff);
  data[27] = (uint8_t)((crc >> 24) & 0xff);
}

static int failures = 0;

static void expect(bool condition, const char* message) {
  if (!condition) {
    printf("FAIL: %s\n", message);
    failures++;
  }
}

static void testFixturesParseAndVerify() {
  for (uint16_t i = 0; i < fixtures::kAllCount; i++) {
    ContainerView view;
    ParseError parseError;
    const bool parsed =
        parseContainer(fixtures::kAll[i].data, fixtures::kAll[i].size, view, parseError);
    if (!parsed) {
      printf("FAIL parse %s: %s\n", fixtures::kAll[i].name, parseErrorName(parseError));
      failures++;
      continue;
    }
    VerifyResult verify;
    if (!verifyContainer(view, kCaps, verify)) {
      printf("FAIL verify %s: %s at %u\n", fixtures::kAll[i].name,
             verifyErrorName(verify.error), (unsigned)verify.offset);
      failures++;
    }
  }
}

static void testBadMagic() {
  uint8_t copy[fixtures::kAll[0].size];
  memcpy(copy, fixtures::kAll[0].data, fixtures::kAll[0].size);
  copy[0] = 'X';
  ContainerView view;
  ParseError error;
  expect(!parseContainer(copy, fixtures::kAll[0].size, view, error), "bad magic must fail");
  expect(error == ParseError::MAGIC, "bad magic error");
}

static void testBadCrc() {
  uint8_t copy[fixtures::kAll[0].size];
  memcpy(copy, fixtures::kAll[0].data, fixtures::kAll[0].size);
  copy[fixtures::kAll[0].size - 2] ^= 0xff;
  ContainerView view;
  ParseError error;
  expect(!parseContainer(copy, fixtures::kAll[0].size, view, error), "bad crc must fail");
  expect(error == ParseError::CRC, "bad crc error");
}

static void testBadJump() {
  uint8_t copy[fixtures::kAll[2].size];
  memcpy(copy, fixtures::kAll[2].data, fixtures::kAll[2].size);
  ContainerView view;
  ParseError parseError;
  expect(parseContainer(copy, fixtures::kAll[2].size, view, parseError), "fixture parses");
  // while_loop: find the first JUMP and point it outside the code.
  uint16_t jump = 0xffff;
  for (uint16_t i = 0; i + 2 < view.header.codeLength; i++) {
    if (view.code[i] == (uint8_t)Opcode::JUMP) {
      jump = i;
      break;
    }
  }
  expect(jump != 0xffff, "while fixture has a jump");
  if (jump != 0xffff) {
    const uint16_t target = (uint16_t)(view.header.codeLength + 8);
    copy[28 + view.constantsSize + jump + 1] = (uint8_t)(target & 0xff);
    copy[28 + view.constantsSize + jump + 2] = (uint8_t)((target >> 8) & 0xff);
    fixCrc(copy, fixtures::kAll[2].size);
    ContainerView patched;
    expect(parseContainer(copy, fixtures::kAll[2].size, patched, parseError), "patched fixture parses");
    VerifyResult verify;
    expect(!verifyContainer(patched, kCaps, verify), "bad jump must fail");
    expect(verify.error == VerifyError::JUMP, "bad jump error");
  }
}

static void testFuzzParse() {
  uint8_t buffer[128];
  for (uint16_t i = 0; i < 10000; i++) {
    const uint16_t size = (uint16_t)(i % sizeof(buffer));
    for (uint16_t j = 0; j < size; j++) {
      buffer[j] = (uint8_t)((i * 31u + j * 17u + 13u) & 0xff);
    }
    ContainerView view;
    ParseError error;
    (void)parseContainer(buffer, size, view, error);
  }
  expect(true, "fuzz parse completed");
}

// ---------------------------------------------------------------------------
// VM tests

static uint16_t g_ports[64];
static char g_display[64];
static char g_log[256];
static bool g_hasFade;
static int32_t g_fadeArgs[3];

static void resetVmState() {
  memset(g_ports, 0, sizeof(g_ports));
  memset(g_display, 0, sizeof(g_display));
  memset(g_log, 0, sizeof(g_log));
  memset(g_fadeArgs, 0, sizeof(g_fadeArgs));
  g_hasFade = false;
  stub_millis = 0;
}

static bool testPortProvider(uint8_t gpio, uint8_t action, uint16_t& value) {
  if (gpio >= sizeof(g_ports) / sizeof(g_ports[0])) return false;
  if (action == PORT_WRITE) {
    g_ports[gpio] = value;
  } else {
    value = g_ports[gpio];
  }
  return true;
}

static bool testDataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  if (strcmp(id, "$display") != 0) return false;
  if (write) {
    if (kind != KIND_STRING) return false;
    uint16_t len = value.stringVal.len;
    if (len >= sizeof(g_display)) len = sizeof(g_display) - 1;
    if (len > 0 && value.stringVal.data) memcpy(g_display, value.stringVal.data, len);
    g_display[len] = '\0';
    return true;
  }
  value.stringVal.data = (uint8_t*)g_display;
  value.stringVal.len = (uint8_t)strlen(g_display);
  return true;
}

static void testLogProvider(const char* message) {
  if (!message) return;
  strncpy(g_log, message, sizeof(g_log) - 1);
  g_log[sizeof(g_log) - 1] = '\0';
}

static bool testFadeHandler(uint8_t count, const Value* params, Value& result, void*) {
  g_hasFade = true;
  for (uint8_t i = 0; i < 3; i++) {
    g_fadeArgs[i] = i < count ? (int32_t)params[i].uintVal : 0;
  }
  result.type = VAL_INT;
  result.intVal = 7;
  return true;
}

static void setupVm(BytecodeVM& vm) {
  vm.setCapabilities(kCaps);
  vm.setPortProvider(testPortProvider);
  vm.setDataProvider(testDataProvider);
  vm.setLogProvider(testLogProvider);
  vm.registerFunction(1, testFadeHandler);
}

static const fixtures::Vector* findFixture(const char* name) {
  for (uint16_t i = 0; i < fixtures::kAllCount; i++) {
    if (strcmp(fixtures::kAll[i].name, name) == 0) return &fixtures::kAll[i];
  }
  return nullptr;
}

static bool runFixture(BytecodeVM& vm, const char* name, uint8_t id, uint8_t updates = 6) {
  const fixtures::Vector* fixture = findFixture(name);
  if (!fixture) {
    expect(false, "fixture not found");
    return false;
  }
  if (!vm.loadScript(id, fixture->data, fixture->size)) {
    expect(false, "fixture did not load");
    return false;
  }
  if (vm.runScript(id) < 0) {
    expect(false, "fixture did not run");
    return false;
  }
  for (uint8_t i = 0; i < updates && vm.isBusy(); i++) {
    vm.update(stub_millis);
  }
  return true;
}

static void testVmSimple() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "simple", 1), "simple runs");
  expect(vm.getUintVar(0) == 5, "simple $u0");
  expect(vm.getUintVar(1) == 8, "simple $u1");
}

static void testVmIfElse() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "if_else", 1), "if_else runs");
  expect(g_ports[2] == 255, "if_else port 2");
}

static void testVmWhile() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "while_loop", 1), "while runs");
  expect(vm.getUintVar(0) == 5, "while $u0");
}

static void testVmTernary() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "ternary", 1), "ternary runs");
  expect(vm.getUintVar(0) == 20, "ternary $u0");
}

static void testVmArray() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "array", 1), "array runs");
  expect(vm.getUintVar(0) == 2, "array $u0");
  expect(vm.getArrayLen(0) == 3, "array length");
}

static void testVmData() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "data", 1), "data runs");
  expect(strcmp(g_display, "hello") == 0, "data display");
}

static void testVmExternal() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "external", 1), "external runs");
  expect(vm.getUintVar(0) == 7, "external result");
  expect(g_hasFade, "fade called");
  expect(g_fadeArgs[0] == 2, "fade arg 0");
  expect(g_fadeArgs[1] == 255, "fade arg 1");
  expect(g_fadeArgs[2] == 1000, "fade arg 2");
}

static void testVmLog() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "log", 1), "log runs");
  expect(strstr(g_log, "hello") != nullptr, "log string");
  expect(strstr(g_log, "42") != nullptr, "log number");
}

static void testVmStrings() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "strings", 1), "strings runs");
  expect(vm.getUintVar(0) == 2, "string length");
  expect(vm.getUintVar(1) == 97, "string ord");
  expect(strcmp(vm.getStringVar(0), "ab") == 0, "string concat");
  expect(strcmp(vm.getStringVar(1), "A") == 0, "string chr");
}

static void testVmArrayOps() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "array_ops", 1), "array_ops runs");
  expect(vm.getUintVar(0) == 9, "array set/get");
  expect(vm.getUintVar(1) == 3, "array len");
  expect(vm.getArrayLen(0) == 3, "array length");
}

static void testVmFloat() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "float_math", 1), "float_math runs");
  expect(vm.getFloatVar(0) == 1.5f, "float f0");
  expect(vm.getFloatVar(1) == 3.0f, "float f1");
  expect(vm.getUintVar(0) == 1, "float compare");
}

static void testVmInt() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "int_math", 1), "int_math runs");
  expect(vm.getIntVar(0) == -5, "int i0");
  expect(vm.getUintVar(0) == 2, "int add");
}

static void testVmBreakContinue() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "break_continue", 1, 12), "break_continue runs");
  expect(vm.getUintVar(0) == 5, "break/continue value");
}

static void testVmCallee() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* call = findFixture("call");
  const fixtures::Vector* callee = findFixture("callee");
  expect(call != nullptr && callee != nullptr, "callee fixtures");
  if (!call || !callee) return;
  expect(vm.loadScript(1, call->data, call->size), "caller loads");
  expect(vm.loadScript(2, callee->data, callee->size), "callee loads");
  vm.runScript(1);
  for (uint8_t i = 0; i < 6; i++) vm.update(stub_millis);
  expect(vm.getUintVar(0) == 30, "call params");
}

static void testVmPortRead() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "port_read", 1), "port_read runs");
  expect(g_ports[2] == 255, "port written");
  expect(vm.getUintVar(0) == 255, "port read");
}

static void testVmDataRead() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(runFixture(vm, "data_read", 1), "data_read runs");
  expect(strcmp(g_display, "hi") == 0, "data written");
  expect(strcmp(vm.getStringVar(0), "hi") == 0, "data read");
}

static void testVmHandlerReturn() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* fixture = findFixture("handler_return");
  expect(fixture != nullptr, "handler_return fixture");
  if (!fixture) return;
  expect(vm.loadScript(1, fixture->data, fixture->size), "handler_return loads");
  vm.runScript(1);
  vm.update(stub_millis);
  int32_t params[2] = {1, 1};
  expect(vm.emitEvent(1, 2, params), "handler_return event");
  vm.update(stub_millis);
  expect(g_ports[2] == 1, "return stops handler");
}

static void testVmHandler() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* fixture = findFixture("handler");
  expect(fixture != nullptr, "handler fixture");
  if (!fixture) return;
  expect(vm.loadScript(1, fixture->data, fixture->size), "handler loads");
  vm.runScript(1);
  vm.update(stub_millis);
  int32_t params[2] = {2, 3};
  expect(vm.emitEvent(1, 2, params), "handler event");
  vm.update(stub_millis);
  expect(vm.getUintVar(0) == 5, "handler sum");
}

static void testVmCall() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* call = findFixture("call");
  const fixtures::Vector* simple = findFixture("simple");
  expect(call != nullptr && simple != nullptr, "call fixtures");
  if (!call || !simple) return;
  expect(vm.loadScript(1, call->data, call->size), "caller loads");
  expect(vm.loadScript(2, simple->data, simple->size), "callee loads");
  vm.runScript(1);
  for (uint8_t i = 0; i < 6; i++) vm.update(stub_millis);
  expect(vm.getUintVar(1) == 8, "callee ran");
}

static void testVmWait() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* fixture = findFixture("wait");
  expect(fixture != nullptr, "wait fixture");
  if (!fixture) return;
  expect(vm.loadScript(1, fixture->data, fixture->size), "wait loads");
  vm.runScript(1);
  vm.update(stub_millis);
  expect(vm.getUintVar(0) == 0, "wait blocks");
  stub_millis = 101;
  vm.update(stub_millis);
  expect(vm.getUintVar(0) == 1, "wait resumes");
}

static void testVmDivZero() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* fixture = findFixture("div_zero");
  expect(fixture != nullptr, "div_zero fixture");
  if (!fixture) return;
  expect(vm.loadScript(1, fixture->data, fixture->size), "div_zero loads");
  vm.runScript(1);
  vm.update(stub_millis);
  expect(vm.lastError() == VmError::RUNTIME, "div zero runtime error");
}

static void testVmBadContainer() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  uint8_t copy[fixtures::kAll[0].size];
  memcpy(copy, fixtures::kAll[0].data, fixtures::kAll[0].size);
  copy[fixtures::kAll[0].size - 2] ^= 0xff;
  expect(!vm.loadScript(1, copy, fixtures::kAll[0].size), "bad container rejected");
  expect(vm.lastError() == VmError::PARSE, "bad container error");
}

static void testVmUnknownEvent() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  expect(!vm.emitEvent((uint8_t)99, 0, nullptr), "unknown event rejected");
}

static void testVmMissingCallee() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* call = findFixture("call");
  expect(call != nullptr, "call fixture");
  if (!call) return;
  expect(vm.loadScript(1, call->data, call->size), "call loads");
  vm.runScript(1);
  for (uint8_t i = 0; i < 4; i++) vm.update(stub_millis);
  expect(vm.lastError() == VmError::NO_SCRIPT, "missing callee error");
}

static void testVmStopAll() {
  BytecodeVM vm;
  setupVm(vm);
  resetVmState();
  const fixtures::Vector* fixture = findFixture("wait");
  expect(fixture != nullptr, "wait fixture");
  if (!fixture) return;
  expect(vm.loadScript(1, fixture->data, fixture->size), "wait loads");
  vm.runScript(1);
  vm.update(stub_millis);
  expect(vm.activeContexts() == 1, "one active context");
  vm.stopAll();
  expect(vm.activeContexts() == 0, "stopAll clears contexts");
}

int main() {
  testFixturesParseAndVerify();
  testBadMagic();
  testBadCrc();
  testBadJump();
  testFuzzParse();

  testVmSimple();
  testVmIfElse();
  testVmWhile();
  testVmTernary();
  testVmArray();
  testVmData();
  testVmExternal();
  testVmLog();
  testVmStrings();
  testVmArrayOps();
  testVmFloat();
  testVmInt();
  testVmBreakContinue();
  testVmCallee();
  testVmPortRead();
  testVmDataRead();
  testVmHandlerReturn();
  testVmHandler();
  testVmCall();
  testVmWait();
  testVmDivZero();
  testVmBadContainer();
  testVmUnknownEvent();
  testVmMissingCallee();
  testVmStopAll();

  if (failures == 0) {
    printf("All bytecode tests passed\n");
    return 0;
  }
  printf("%d bytecode test(s) failed\n", failures);
  return 1;
}
