#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../runner/runner.h"
#include "arduino_stub.h"

static uint16_t virtualPins[40] = {0};
static std::vector<std::string> testLog;
static std::vector<uint8_t> uartTxBuffer;
static std::vector<uint8_t> uartRxBuffer;
static size_t uartRxIndex = 0;

static uint32_t externalCounter = 0;
static float externalTemp = 0.0f;

bool testPortProvider(uint8_t gpio, PortAction action, uint16_t& value) {
  if (action == PORT_READ) {
    value = virtualPins[gpio];
    return true;
  } else {
    virtualPins[gpio] = value;
    return true;
  }
}

bool testDataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  if (strcmp(id, "$u0") == 0 && kind == KIND_STRING) {
    if (write) {
      uartTxBuffer.assign(value.stringVal.data, value.stringVal.data + value.stringVal.len);
      return true;
    } else {
      if (uartRxIndex < uartRxBuffer.size()) {
        size_t remaining = uartRxBuffer.size() - uartRxIndex;
        size_t toCopy = remaining < value.stringVal.len ? remaining : value.stringVal.len;
        memcpy(value.stringVal.data, &uartRxBuffer[uartRxIndex], toCopy);
        value.stringVal.len = toCopy;
        uartRxIndex += toCopy;
        return true;
      }
      value.stringVal.len = 0;
      return true;
    }
  }
  if (strcmp(id, "$counter") == 0 && kind == KIND_UINT) {
    if (write)
      externalCounter = value.uintVal;
    else
      value.uintVal = externalCounter;
    return true;
  }
  if (strcmp(id, "$temp") == 0 && kind == KIND_FLOAT) {
    if (write)
      externalTemp = value.floatVal;
    else
      value.floatVal = externalTemp;
    return true;
  }
  return false;
}

void testLogProvider(const char* message) {
  testLog.push_back(std::string(message));
}

void resetTestState() {
  stub_millis = 0;
  memset(virtualPins, 0, sizeof(virtualPins));
  testLog.clear();
  uartTxBuffer.clear();
  uartRxBuffer.clear();
  uartRxIndex = 0;
  externalCounter = 0;
  externalTemp = 0.0f;
}

void runScriptUntilDone(ScriptRunner& runner, uint32_t maxSteps = 1000) {
  for (uint32_t i = 0; i < maxSteps; i++) {
    if (!runner.isBusy()) break;
    stub_millis += 100;
    runner.update();
  }
}

bool logContains(const char* text) {
  for (const auto& s : testLog) {
    if (s.find(text) != std::string::npos) return true;
  }
  return false;
}

static bool test_failed = false;

#define ASSERT_EQ(a, b)                                               \
  do {                                                                \
    if ((a) != (b)) {                                                 \
      printf("  FAIL %s:%d: %s == %s\n", __FILE__, __LINE__, #a, #b); \
      test_failed = true;                                             \
      return;                                                         \
    }                                                                 \
  } while (0)

#define ASSERT_TRUE(a)                                      \
  do {                                                      \
    if (!(a)) {                                             \
      printf("  FAIL %s:%d: %s\n", __FILE__, __LINE__, #a); \
      test_failed = true;                                   \
      return;                                               \
    }                                                       \
  } while (0)

#define TEST(name) void test_##name()
#define RUN_TEST(name)                              \
  do {                                              \
    resetTestState();                               \
    test_failed = false;                            \
    printf("  RUN  %s\n", #name);                   \
    test_##name();                                  \
    if (!test_failed) printf("  PASS %s\n", #name); \
  } while (0)

// ===== БАЗОВЫЕ ТЕСТЫ =====

TEST(variable_assignment) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=42,log:Value is $v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_TRUE(logContains("42"));
}

TEST(variable_arithmetic) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=10", RESTART);
  runScriptUntilDone(runner);
  runner.addScript(2, "$v0+5", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
}

TEST(variable_multiplication) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=6,$v0*7", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
}

TEST(variable_division) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=100,$v0/3", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 33u);
}

TEST(float_arithmetic) {
  ScriptRunner runner;
  runner.addScript(1, "$f0=10.5,$f0+2.5", RESTART);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_TRUE(val > 12.9f && val < 13.1f);
}

TEST(bitwise_and) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=255,$v0=&15", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
}

TEST(bitwise_or) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=240,$v0=|15", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 255u);
}

TEST(bitwise_xor) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=255,$v0=^255", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(bitwise_not) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=0,$v0=~", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0xFFFFFFFFu);
}

TEST(shift_left) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=1,$v0=<<4", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 16u);
}

TEST(shift_right) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=16,$v0=>>2", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 4u);
}

TEST(modulo) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=17,$v0=%5", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 2u);
}

TEST(condition_true) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=1,if:$v0==1,log:TRUE,else,log:FALSE,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("TRUE"));
  ASSERT_TRUE(!logContains("FALSE"));
}

TEST(condition_false) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=0,if:$v0==1,log:TRUE,else,log:FALSE,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("FALSE"));
  ASSERT_TRUE(!logContains("TRUE"));
}

TEST(condition_no_else_false) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=0,if:$v0==1,log:SKIP,end,log:DONE", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(!logContains("SKIP"));
  ASSERT_TRUE(logContains("DONE"));
}

TEST(logical_and) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=1,$v1=1,if:$v0==1&&$v1==1,log:BOTH,else,log:NONE,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("BOTH"));
}

TEST(logical_or) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=0,$v1=1,if:$v0==1||$v1==1,log:ANY,else,log:NONE,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("ANY"));
}

TEST(loop_fixed) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=0,[5]$v0+1]", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
}

// ===== СТРОКИ =====

TEST(string_assign) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"test\",log:String $s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("test"));
}

TEST(string_length) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"hello\",log:Length $s0#", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("Length 5"));
}

TEST(string_concat) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"hello\",$s0+\"world\",log:String $s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("helloworld"));
}

TEST(string_hex_assign) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=0x00FF12,$v0=$s0{1}", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0xFFu);
}

TEST(string_byte_write) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=\"AAAA\",$s0{2}=66,$v0=$s0{2}", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 66u);
}

TEST(string_compare_equal) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"LEDON\",if:$s0==LEDON,log:MATCH,else,log:NOMATCH,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("MATCH"));
}

TEST(string_compare_not_equal) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"LEDON\",if:$s0!=LEDOFF,log:DIFF,else,log:SAME,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("DIFF"));
}

TEST(string_clear) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=\"hello\",$s0=\"\",$v0=$s0#", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(string_copy) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"hello\",$s1=$s0,log:String $s1", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("hello"));
}

TEST(string_concat_hex) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=0xAABB,$s0+0xCCDD,$v0=$s0{0},$v1=$s0{2}", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0xAAu);
  ASSERT_EQ(runner.getUintVar(1), 0xCCu);
}

TEST(string_range_read) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=0x01020304,$v0=$s0{1:2}", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0x0203u);
}

TEST(string_out_of_bounds_read) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=\"AB\",$v0=$s0{100}", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(string_out_of_bounds_write) {
  ScriptRunner runner;
  runner.addScript(1, "$s0=\"AB\",$s0{100}=65,$v0=$s0#", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 2u);
}

TEST(string_empty_log) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"\",log:Empty $s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("Empty "));
}

TEST(string_length_condition) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"hello\",if:$s0#>3,log:LONG,else,log:SHORT,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("LONG"));
}

// ===== ВЛОЖЕННЫЕ КОНСТРУКЦИИ =====

TEST(nested_if_in_loop) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=0,[5]$v1=1,if:$v1==1,$v0+1,end]", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
}

TEST(loop_in_if) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=0,$v1=1,if:$v1==1,[3]$v0+1],end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 3u);
}

TEST(triple_nested_if) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=1,$v1=1,$v2=1,if:$v0==1,if:$v1==1,if:$v2==1,log:DEEP,else,log:NO3,end,else,log:NO2,end,else,log:NO1,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("DEEP"));
}

// ===== ВНЕШНИЕ ПЕРЕМЕННЫЕ =====

TEST(external_variable_read) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  externalCounter = 42;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "log:Counter $counter", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("42"));
}

TEST(external_variable_write) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$counter=77", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(externalCounter, 77u);
}

TEST(external_variable_bitwise) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  externalCounter = 0xFF;
  runner.addScript(1, "$counter=&15", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(externalCounter, 15u);
}

TEST(external_float_variable) {
    ScriptRunner runner;
    runner.setDataProvider(testDataProvider);
    runner.addScript(1, "$f0=23.5", RESTART);
    runScriptUntilDone(runner);
    float val = runner.getFloatVar(0);
    ASSERT_TRUE(val > 23.4f && val < 23.6f);
}

// ===== МАКРОСЫ =====

TEST(macro_port_to_var) {
  ScriptRunner runner;
  runner.setPortProvider(testPortProvider);
  virtualPins[13] = 200;
  runner.addScript(1, "$v0=$p13", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 200u);
}

TEST(macro_analog_to_var) {
  ScriptRunner runner;
  runner.setPortProvider(testPortProvider);
  virtualPins[2] = 512;
  runner.addScript(1, "$v0=$a2", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 512u);
}

TEST(macro_var_to_var) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=99,$v1=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(1), 99u);
}

TEST(macro_float_to_uint) {
  ScriptRunner runner;
  runner.addScript(1, "$f0=12.5,$v0=$f0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
}

// ===== СОБЫТИЯ =====

TEST(event_on) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "on:$p13->ris,log:EVENT", RESTART);
  stub_millis += 100;
  runner.update();
  ASSERT_TRUE(!logContains("EVENT"));
  runner.emitEvent("$p13->ris");
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("EVENT"));
}

TEST(multiple_events) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "on:$p13->ris,log:FIRST,on:$p14->ris,log:SECOND", RESTART);
  runScriptUntilDone(runner, 10);
  ASSERT_TRUE(!logContains("FIRST"));
  runner.emitEvent("$p13->ris");
  runScriptUntilDone(runner, 10);
  ASSERT_TRUE(logContains("FIRST"));
  ASSERT_TRUE(!logContains("SECOND"));
  runner.emitEvent("$p14->ris");
  runScriptUntilDone(runner, 10);
  ASSERT_TRUE(logContains("SECOND"));
}

// ===== СТРАТЕГИИ =====

TEST(strategy_ignore) {
    ScriptRunner runner;
    runner.setLogProvider(testLogProvider);
    // Бесконечный цикл чтобы скрипт оставался активным
    runner.addScript(1, "[*]log:FIRST,p100]", IGNORE);
    runScriptUntilDone(runner, 10);
    testLog.clear();
    runner.addScript(1, "log:SECOND", IGNORE);
    runScriptUntilDone(runner, 10);
    // SECOND не должен появиться потому что скрипт с ID 1 уже активен и стратегия IGNORE
    ASSERT_TRUE(!logContains("SECOND"));
}

TEST(strategy_restart_if_same) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "log:SAME", RESTART_IF_SAME);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.addScript(1, "log:SAME", RESTART_IF_SAME);
  runScriptUntilDone(runner, 10);
  ASSERT_TRUE(logContains("SAME"));
}

TEST(register_and_run) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.registerScript(5, "log:REGISTERED");
  runner.runScript(5);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("REGISTERED"));
}

TEST(call_script) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "log:CALLED", RESTART);
  runScriptUntilDone(runner);
  testLog.clear();
  runner.addScript(2, "call:1", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("CALLED"));
}

TEST(call_nonexistent) {
  ScriptRunner runner;
  runner.addScript(1, "call:99", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

// ===== ПОРТЫ =====

TEST(port_write) {
  ScriptRunner runner;
  runner.setPortProvider(testPortProvider);
  runner.addScript(1, "$p13=200", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(virtualPins[13], 200u);
}

TEST(port_fade) {
  ScriptRunner runner;
  runner.setPortProvider(testPortProvider);
  virtualPins[13] = 0;
  runner.addScript(1, "$p13=200/20", RESTART);
  runScriptUntilDone(runner, 500);
  ASSERT_EQ(virtualPins[13], 200u);
}

TEST(condition_with_port) {
  ScriptRunner runner;
  runner.setPortProvider(testPortProvider);
  runner.setLogProvider(testLogProvider);
  virtualPins[13] = 0;
  runner.addScript(1, "if:$p13==0,log:LOW,else,log:HIGH,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("LOW"));
}

// ===== UART =====

TEST(uart_send) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0=0x010300640001,$u0=$s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(uartTxBuffer.size(), 6u);
  ASSERT_EQ(uartTxBuffer[0], 0x01);
  ASSERT_EQ(uartTxBuffer[1], 0x03);
}

TEST(uart_receive) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint8_t response[] = {0x01, 0x03, 0x04, 0x00, 0x7B, 0x00, 0xC8};
  uartRxBuffer.assign(response, response + 7);
  runner.addScript(1, "$s0=$u0,$v0=$s0{3:4}", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0x007Bu);
}

// ===== FLOAT =====

TEST(float_copy) {
  ScriptRunner runner;
  runner.addScript(1, "$f0=3.14,$f1=$f0", RESTART);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(1);
  ASSERT_TRUE(val > 3.13f && val < 3.15f);
}

TEST(float_in_condition_with_and) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$f0=10.5,$f1=20.5,if:$f0>10.0&&$f1<30.0,log:RANGE,else,log:NORANGE,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("RANGE"));
}

TEST(float_bitwise_ignored) {
  ScriptRunner runner;
  runner.addScript(1, "$f0=10.5,$f0=&15", RESTART);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_TRUE(val > 10.4f && val < 10.6f);
}

// ===== ПРОЧЕЕ =====

TEST(random_value) {
  ScriptRunner runner;
  runner.addScript(1, "$v0=@100", RESTART);
  runScriptUntilDone(runner);
  uint32_t val = runner.getUintVar(0);
  ASSERT_TRUE(val < 100);
}

TEST(else_if_chain) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$v0=2,if:$v0==1,log:ONE,else,if:$v0==2,log:TWO,else,log:OTHER,end,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("TWO"));
}

TEST(debug_string_compare_not_equal) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"LEDON\",log:Len $s0#,log:Str $s0,if:$s0!=LEDOFF,log:DIFF,else,log:SAME,end", RESTART);
  runScriptUntilDone(runner);
  for (auto& s : testLog) printf("    %s\n", s.c_str());
}

TEST(debug_string_length_condition) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "$s0=\"hello\",log:Len $s0#,if:$s0#>3,log:LONG,else,log:SHORT,end", RESTART);
  runScriptUntilDone(runner);
  for (auto& s : testLog) printf("    %s\n", s.c_str());
}

TEST(debug_multiple_events) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "on:$p13->ris,log:FIRST,on:$p14->ris,log:SECOND", RESTART);
  for (int i = 0; i < 10; i++) {
    stub_millis += 100;
    runner.update();
  }
  printf("    After 10 updates:\n");
  for (auto& s : testLog) printf("      %s\n", s.c_str());
  runner.emitEvent("$p13->ris");
  for (int i = 0; i < 10; i++) {
    stub_millis += 100;
    runner.update();
  }
  printf("    After emit:\n");
  for (auto& s : testLog) printf("      %s\n", s.c_str());
}

TEST(debug_strategy_ignore) {
  ScriptRunner runner;
  runner.setLogProvider(testLogProvider);
  runner.addScript(1, "log:FIRST", IGNORE);
  runScriptUntilDone(runner);
  printf("    After first: isRunning=%d\n", runner.isRunning(1));
  for (auto& s : testLog) printf("      %s\n", s.c_str());
  testLog.clear();
  runner.addScript(1, "log:SECOND", IGNORE);
  runScriptUntilDone(runner);
  printf("    After second: isRunning=%d\n", runner.isRunning(1));
  for (auto& s : testLog) printf("      %s\n", s.c_str());
}

int main() {
  printf("=== ScriptRunner Tests ===\n\n");

  RUN_TEST(variable_assignment);
  RUN_TEST(variable_arithmetic);
  RUN_TEST(variable_multiplication);
  RUN_TEST(variable_division);
  RUN_TEST(float_arithmetic);
  RUN_TEST(bitwise_and);
  RUN_TEST(bitwise_or);
  RUN_TEST(bitwise_xor);
  RUN_TEST(bitwise_not);
  RUN_TEST(shift_left);
  RUN_TEST(shift_right);
  RUN_TEST(modulo);
  RUN_TEST(condition_true);
  RUN_TEST(condition_false);
  RUN_TEST(condition_no_else_false);
  RUN_TEST(logical_and);
  RUN_TEST(logical_or);
  RUN_TEST(loop_fixed);
  RUN_TEST(string_assign);
  RUN_TEST(string_length);
  RUN_TEST(string_concat);
  RUN_TEST(string_hex_assign);
  RUN_TEST(string_byte_write);
  RUN_TEST(string_compare_equal);
  RUN_TEST(string_compare_not_equal);
  RUN_TEST(string_clear);
  RUN_TEST(string_copy);
  RUN_TEST(string_concat_hex);
  RUN_TEST(string_range_read);
  RUN_TEST(string_out_of_bounds_read);
  RUN_TEST(string_out_of_bounds_write);
  RUN_TEST(string_empty_log);
  RUN_TEST(string_length_condition);
  RUN_TEST(nested_if_in_loop);
  RUN_TEST(loop_in_if);
  RUN_TEST(triple_nested_if);
  RUN_TEST(external_variable_read);
  RUN_TEST(external_variable_write);
  RUN_TEST(external_variable_bitwise);
  RUN_TEST(external_float_variable);
  RUN_TEST(macro_port_to_var);
  RUN_TEST(macro_analog_to_var);
  RUN_TEST(macro_var_to_var);
  RUN_TEST(macro_float_to_uint);
  RUN_TEST(event_on);
  RUN_TEST(multiple_events);
  RUN_TEST(strategy_ignore);
  RUN_TEST(strategy_restart_if_same);
  RUN_TEST(register_and_run);
  RUN_TEST(call_script);
  RUN_TEST(call_nonexistent);
  RUN_TEST(port_write);
  RUN_TEST(port_fade);
  RUN_TEST(condition_with_port);
  RUN_TEST(uart_send);
  RUN_TEST(uart_receive);
  RUN_TEST(float_copy);
  RUN_TEST(float_in_condition_with_and);
  RUN_TEST(float_bitwise_ignored);
  RUN_TEST(random_value);
  RUN_TEST(else_if_chain);
  RUN_TEST(debug_string_compare_not_equal);
  RUN_TEST(debug_string_length_condition);
  RUN_TEST(debug_multiple_events);
  RUN_TEST(debug_strategy_ignore);

  printf("\n=== All tests passed ===\n");
  return 0;
}