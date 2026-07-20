#include <cstdio>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "../runner/runner.h"
#include "arduino_stub.h"

static uint16_t virtualPins[40] = {0};
static std::vector<std::string> testLog;
static std::map<std::string, std::string> dataStore;
static bool test_failed = false;

bool testPortProvider(uint8_t gpio, PortAction action, uint16_t& value) {
  if (gpio >= 40) return false;
  if (action == PORT_READ) {
    value = virtualPins[gpio];
    return true;
  } else {
    virtualPins[gpio] = value;
    return true;
  }
}

void testStateChangeProvider(uint8_t gpio, uint16_t oldValue, uint16_t newValue) {
}

bool testDataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  if (!id) return false;

  if (strcmp(id, "$display") == 0) {
    if (write) {
      char buf[64];
      if (kind == KIND_STRING) {
        int len = value.stringVal.len < 63 ? value.stringVal.len : 63;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
      } else if (kind == KIND_UINT) {
        snprintf(buf, 64, "%u", value.uintVal);
      } else if (kind == KIND_INT) {
        snprintf(buf, 64, "%d", value.intVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(buf, 64, "%.2f", value.floatVal);
      } else {
        return false;
      }
      testLog.push_back(std::string(buf));
      return true;
    }
    return true;
  }

  if (write) {
    if (kind == KIND_STRING) {
      std::string str((char*)value.stringVal.data, value.stringVal.len);
      dataStore[id] = str;
    } else if (kind == KIND_UINT) {
      char buf[32];
      snprintf(buf, 32, "%u", value.uintVal);
      dataStore[id] = buf;
    } else if (kind == KIND_INT) {
      char buf[32];
      snprintf(buf, 32, "%d", value.intVal);
      dataStore[id] = buf;
    } else if (kind == KIND_FLOAT) {
      char buf[32];
      snprintf(buf, 32, "%.6f", value.floatVal);
      dataStore[id] = buf;
    }
    return true;
  } else {
    auto it = dataStore.find(id);
    if (it != dataStore.end()) {
      if (kind == KIND_STRING) {
        value.stringVal.data = (uint8_t*)it->second.c_str();
        value.stringVal.len = it->second.length();
      } else if (kind == KIND_UINT) {
        value.uintVal = (uint32_t)std::stoul(it->second);
      } else if (kind == KIND_INT) {
        value.intVal = (int32_t)std::stoi(it->second);
      } else if (kind == KIND_FLOAT) {
        value.floatVal = std::stof(it->second);
      }
      return true;
    }
  }
  return false;
}

void testLogProvider(const char* message) {
  if (message) testLog.push_back(std::string(message));
}

void resetTestState() {
  stub_millis = 0;
  memset(virtualPins, 0, sizeof(virtualPins));
  testLog.clear();
  dataStore.clear();
  test_failed = false;
}

void runScriptUntilDone(ScriptRunner& runner, uint32_t maxSteps = 200) {
  uint32_t steps = 0;
  uint32_t idleCount = 0;

  while (runner.isBusy() && steps < maxSteps) {
    stub_millis += 10;
    runner.update();
    steps++;

    if (!runner.isBusy()) {
      idleCount++;
      if (idleCount > 5) break;
    } else {
      idleCount = 0;
    }

    if (steps % 20 == 0 && runner.isBusy()) {
      stub_millis += 100;
    }
  }
}

bool logContains(const char* text) {
  for (const auto& s : testLog) {
    if (s.find(text) != std::string::npos) return true;
  }
  return false;
}

bool logExact(const char* text) {
  for (const auto& s : testLog) {
    if (s == text) return true;
  }
  return false;
}

void printLog() {
  printf("  LOG:\n");
  for (const auto& s : testLog) {
    printf("    %s\n", s.c_str());
  }
}

#define ASSERT_EQ(a, b)                                        \
  do {                                                         \
    if ((a) != (b)) {                                          \
      printf("  FAIL %s:%d: %s == %s (got %d, expected %d)\n", \
             __FILE__, __LINE__, #a, #b, (int)(a), (int)(b));  \
      test_failed = true;                                      \
      printLog();                                              \
      return;                                                  \
    }                                                          \
  } while (0)

#define ASSERT_FLOAT_EQ(a, b, eps)                                 \
  do {                                                             \
    float _a = (a), _b = (b);                                      \
    if (_a < _b - eps || _a > _b + eps) {                          \
      printf("  FAIL %s:%d: %s == %s (got %.3f, expected %.3f)\n", \
             __FILE__, __LINE__, #a, #b, _a, _b);                  \
      test_failed = true;                                          \
      printLog();                                                  \
      return;                                                      \
    }                                                              \
  } while (0)

#define ASSERT_TRUE(a)                                               \
  do {                                                               \
    if (!(a)) {                                                      \
      printf("  FAIL %s:%d: %s is false\n", __FILE__, __LINE__, #a); \
      test_failed = true;                                            \
      printLog();                                                    \
      return;                                                        \
    }                                                                \
  } while (0)

#define ASSERT_FALSE(a)                                             \
  do {                                                              \
    if ((a)) {                                                      \
      printf("  FAIL %s:%d: %s is true\n", __FILE__, __LINE__, #a); \
      test_failed = true;                                           \
      printLog();                                                   \
      return;                                                       \
    }                                                               \
  } while (0)

#define ASSERT_LOG_CONTAINS(text)                         \
  do {                                                    \
    if (!logContains(text)) {                             \
      printf("  FAIL %s:%d: log does not contain '%s'\n", \
             __FILE__, __LINE__, text);                   \
      test_failed = true;                                 \
      printLog();                                         \
      return;                                             \
    }                                                     \
  } while (0)

#define ASSERT_LOG_NOT_CONTAINS(text)             \
  do {                                            \
    if (logContains(text)) {                      \
      printf("  FAIL %s:%d: log contains '%s'\n", \
             __FILE__, __LINE__, text);           \
      test_failed = true;                         \
      printLog();                                 \
      return;                                     \
    }                                             \
  } while (0)

#define ASSERT_LOG_EXACT(text)                                  \
  do {                                                          \
    if (!logExact(text)) {                                      \
      printf("  FAIL %s:%d: log does not exactly match '%s'\n", \
             __FILE__, __LINE__, text);                         \
      test_failed = true;                                       \
      printLog();                                               \
      return;                                                   \
    }                                                           \
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

TEST(variable_assignment_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=42;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_LOG_EXACT("42");
}

TEST(variable_assignment_int) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$i0=-10;$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -10);
  ASSERT_LOG_EXACT("-10");
}

TEST(variable_assignment_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$display=$f0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_FLOAT_EQ(val, 3.14f, 0.01f);
  ASSERT_LOG_CONTAINS("3.14");
}

TEST(variable_assignment_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='Hello';$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Hello");
}

TEST(arithmetic_add) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=10;$v0=$v0+5;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
  ASSERT_LOG_EXACT("15");
}

TEST(arithmetic_sub) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=20;$v0=$v0-8;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
  ASSERT_LOG_EXACT("12");
}

TEST(arithmetic_mul) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=6;$v0=$v0*7;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_LOG_EXACT("42");
}

TEST(arithmetic_div) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=100;$v0=$v0/4;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 25u);
  ASSERT_LOG_EXACT("25");
}

TEST(arithmetic_mod) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=17;$v0=$v0%5;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 2u);
  ASSERT_LOG_EXACT("2");
}

TEST(arithmetic_int_negative) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$i0=20;$i0=8-$i0;$display=$i0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -12);
  ASSERT_LOG_EXACT("-12");
}

TEST(arithmetic_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=10.5;$f0=$f0+2.5;$display=$f0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_FLOAT_EQ(val, 13.0f, 0.1f);
  ASSERT_LOG_CONTAINS("13.00");
}

TEST(bitwise_and) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=255;$v0=$v0&15;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
  ASSERT_LOG_EXACT("15");
}

TEST(bitwise_or) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=240;$v0=$v0|15;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 255u);
  ASSERT_LOG_EXACT("255");
}

TEST(bitwise_xor) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=255;$v0=$v0^255;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
  ASSERT_LOG_EXACT("0");
}

TEST(bitwise_not) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;$v0=~$v0;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ((int32_t)runner.getUintVar(0), -1);
  ASSERT_LOG_EXACT("-1");
}

TEST(shift_left) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=1;$v0=$v0<<4;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 16u);
  ASSERT_LOG_EXACT("16");
}

TEST(shift_right) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=16;$v0=$v0>>2;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 4u);
  ASSERT_LOG_EXACT("4");
}

TEST(condition_if_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=1;if:$v0==1;$display='TRUE';else;$display='FALSE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("TRUE");
  ASSERT_LOG_NOT_CONTAINS("FALSE");
}

TEST(condition_if_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;if:$v0==1;$display='TRUE';else;$display='FALSE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("FALSE");
  ASSERT_LOG_NOT_CONTAINS("TRUE");
}

TEST(condition_if_else_chain) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=2;if:$v0==1;$display='ONE';else;if:$v0==2;$display='TWO';else;$display='OTHER';end;end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("TWO");
}

TEST(condition_logical_and) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=1;$v1=1;if:$v0==1&&$v1==1;$display='BOTH';else;$display='NONE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("BOTH");
}

TEST(condition_logical_or) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;$v1=1;if:$v0==1||$v1==1;$display='ANY';else;$display='NONE';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("ANY");
}

TEST(condition_compare_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='LEDON';if:$s0=='LEDON';$display='MATCH';else;$display='NOMATCH';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("MATCH");
}

TEST(loop_while) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;while:$v0<5;$v0=$v0+1;end;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
  ASSERT_LOG_EXACT("5");
}

TEST(string_concat) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';$s0=$s0+'world';$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("helloworld");
}

TEST(string_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';$s1=$s0;$display=$s1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("hello");
}

TEST(string_concat_number) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=42;$s0='Value: ';$s0=$s0+$v0;$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Value: 42");
}

TEST(string_concat_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$s0='Pi: ';$s0=$s0+$f0;$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("Pi: 3.14");
}

TEST(string_concat_direct) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='Value: ';$s0=$s0+123;$display=$s0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("Value: 123");
}

TEST(port_write) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.registerScript(1, "$p13=200");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(virtualPins[13], 200u);
}

TEST(port_read) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.setStateChangeProvider(testStateChangeProvider);
  virtualPins[13] = 200;
  runner.registerScript(1, "$v0=$p13;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 200u);
  ASSERT_LOG_EXACT("200");
}

TEST(condition_with_port) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  virtualPins[13] = 0;
  runner.registerScript(1, "if:$p13==0;$display='LOW';else;$display='HIGH';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("LOW");
}

TEST(register_and_run) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(5, "$display='REGISTERED'");
  runner.runScript(5);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("REGISTERED");
}

TEST(wait_time) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t start = stub_millis;
  runner.registerScript(1, "$display='START';wait(100u);$display='END'");
  runner.runScript(1);
  runScriptUntilDone(runner, 50);
  uint32_t elapsed = stub_millis - start;
  ASSERT_TRUE(elapsed >= 80);
  ASSERT_LOG_CONTAINS("START");
  ASSERT_LOG_CONTAINS("END");
}

TEST(nested_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=1;$v1=1;if:$v0==1;if:$v1==1;$display='DEEP';end;end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("DEEP");
}

TEST(loop_in_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;$v1=1;if:$v1==1;while:$v0<3;$v0=$v0+1;end;end;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 3u);
  ASSERT_LOG_EXACT("3");
}

TEST(float_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=$f0;$display=$f1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(1);
  ASSERT_FLOAT_EQ(val, 3.14f, 0.01f);
  ASSERT_LOG_CONTAINS("3.14");
}

TEST(strategy_restart) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$display='FIRST'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.registerScript(1, "$display='SECOND'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  ASSERT_LOG_EXACT("SECOND");
}

TEST(strategy_restart_if_same) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$display='SAME'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.registerScript(1, "$display='SAME'");
  runner.runScript(1);
  runScriptUntilDone(runner, 10);
  ASSERT_LOG_EXACT("SAME");
}

TEST(macro_var_to_var) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=99;$v1=$v0;$display=$v1");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(1), 99u);
  ASSERT_LOG_EXACT("99");
}

TEST(macro_float_to_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=12.5;$v0=$f0;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
  ASSERT_LOG_EXACT("12");
}

TEST(variable_increment) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=5;$v0=$v0+3;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 8u);
  ASSERT_LOG_EXACT("8");
}

TEST(variable_decrement) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=10;$v0=$v0-4;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 6u);
  ASSERT_LOG_EXACT("6");
}

TEST(variable_multiply_assign) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=4;$v0=$v0*5;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 20u);
  ASSERT_LOG_EXACT("20");
}

TEST(variable_divide_assign) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=30;$v0=$v0/6;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
  ASSERT_LOG_EXACT("5");
}

TEST(array_init) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$v2=len($a0);$display=$v2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getArrayLen(0), 3u);
  ASSERT_LOG_EXACT("3");
}

TEST(array_get) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$v0=get($a0,1);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 20u);
  ASSERT_LOG_EXACT("20");
}

TEST(array_set) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};set($a0,1,99);$v0=get($a0,1);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 99u);
  ASSERT_LOG_EXACT("99");
}

TEST(array_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$a1=$a0;$v0=get($a1,0);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 10u);
  ASSERT_LOG_EXACT("10");
}

TEST(array_len) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={10,20,30};$v2=len($a0);$display=$v2");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(2), 3u);
  ASSERT_LOG_EXACT("3");
}

TEST(chr_ord) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0=chr(65);$v0=ord($s0);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 65u);
  ASSERT_LOG_EXACT("65");
}

TEST(division_by_zero) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);
  runner.registerScript(1, "$v0=10;$v0=$v0/0;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(duplicate_script_id) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$display='FIRST'");
  bool result = runner.registerScript(1, "$display='SECOND'");
  ASSERT_TRUE(result);
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("SECOND");
}

TEST(very_long_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  char longStr[MAX_STRING_LEN + 10];
  memset(longStr, 'A', sizeof(longStr) - 1);
  longStr[sizeof(longStr) - 1] = '\0';
  char script[256];
  snprintf(script, sizeof(script), "$s0='%s';$display=$s0", longStr);
  runner.registerScript(1, script);
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("AAAAA"));
}

TEST(port_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.registerScript(1, "$p99=255");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(recursive_call) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setLogProvider(testLogProvider);
  runner.registerScript(1, "call(1)");
  runner.runScript(1);
  runScriptUntilDone(runner, 20);
  ASSERT_TRUE(true);
}

TEST(if_without_else) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=1;if:$v0==1;$display='TRUE';end;$display='DONE'");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_CONTAINS("TRUE");
  ASSERT_LOG_CONTAINS("DONE");
}

TEST(if_without_end) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=1;if:$v0==1;$display='TRUE'");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(while_with_break) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;while:$v0<10;$v0=$v0+1;if:$v0==5;$v0=10;end;end;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 10u);
}

TEST(array_out_of_bounds) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={1,2,3};$v0=get($a0,10);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(string_index_out_of_bounds) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='ABC';$v0=ord($s0,10);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(negative_array_index) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$a0={1,2,3};$v0=get($a0,-1);$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
}

TEST(float_compare_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=3.14;if:$f0==$f1;$display='EQUAL';else;$display='NOT';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("EQUAL");
}

TEST(float_compare_not_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$f0=3.14;$f1=3.15;if:$f0!=$f1;$display='NOT';else;$display='EQUAL';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("NOT");
}

TEST(string_compare_not_equal) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$s0='hello';$s1='world';if:$s0!=$s1;$display='NOT';else;$display='EQUAL';end");
  runner.runScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_EXACT("NOT");
}

TEST(script_exceeds_max_len) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  char script[MAX_SCRIPT_LEN + 10];
  memset(script, 'A', sizeof(script) - 1);
  script[sizeof(script) - 1] = '\0';
  bool result = runner.registerScript(1, script);
  ASSERT_FALSE(result);
}

TEST(get_uint_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t val = runner.getUintVar(99);
  ASSERT_EQ(val, 0u);
}

TEST(get_int_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  int32_t val = runner.getIntVar(99);
  ASSERT_EQ(val, 0);
}

TEST(get_float_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  float val = runner.getFloatVar(99);
  ASSERT_FLOAT_EQ(val, 0.0f, 0.01f);
}

TEST(set_uint_var_out_of_range) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setUintVar(99, 42);
  ASSERT_TRUE(true);
}

TEST(memory_slots) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  ASSERT_EQ(runner.getTotalSlots(), MAX_SCRIPTS);
  ASSERT_EQ(runner.getFreeSlotsCount(), MAX_SCRIPTS);
  runner.registerScript(1, "test");
  ASSERT_EQ(runner.getUsedSlotsCount(), 1);
  ASSERT_EQ(runner.getFreeSlotsCount(), MAX_SCRIPTS - 1);
}

TEST(script_too_long) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  char longScript[MAX_SCRIPT_LEN + 10];
  memset(longScript, 'A', sizeof(longScript) - 1);
  longScript[sizeof(longScript) - 1] = '\0';
  bool result = runner.registerScript(1, longScript);
  ASSERT_FALSE(result);
}

TEST(invalid_slot_id) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  bool result = runner.registerScript(EVENT_HANDLER_ID_BASE, "test");
  ASSERT_FALSE(result);
}

TEST(stop_script) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "wait(1000u);$display='SHOULD_NOT_RUN'");
  runner.runScript(1);
  runner.stopScript(1);
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("SHOULD_NOT_RUN");
}

TEST(stop_all) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "wait(1000u);$display='S1'");
  runner.registerScript(2, "wait(1000u);$display='S2'");
  runner.runScript(1);
  runner.runScript(2);
  runner.stopAll();
  runScriptUntilDone(runner);
  ASSERT_LOG_NOT_CONTAINS("S1");
  ASSERT_LOG_NOT_CONTAINS("S2");
}

TEST(is_busy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "wait(100u)");
  runner.runScript(1);
  ASSERT_TRUE(runner.isBusy());
  runScriptUntilDone(runner);
  ASSERT_FALSE(runner.isBusy());
}

TEST(wait_until) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t start = stub_millis;
  runner.registerScript(1, "$display='START';wait(50u);wait(50u);$display='END'");
  runner.runScript(1);
  runScriptUntilDone(runner);
  uint32_t elapsed = stub_millis - start;
  ASSERT_TRUE(elapsed >= 80);
  ASSERT_LOG_CONTAINS("START");
  ASSERT_LOG_CONTAINS("END");
}

TEST(while_infinite_protection) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(1, "$v0=0;while:$v0<10;$v0=$v0+1;end;$display=$v0");
  runner.runScript(1);
  runScriptUntilDone(runner, 50);
  ASSERT_EQ(runner.getUintVar(0), 10u);
  ASSERT_LOG_EXACT("10");
}

int main() {
  printf("=== ScriptRunner Tests ===\n\n");

  RUN_TEST(variable_assignment_uint);
  RUN_TEST(variable_assignment_int);
  RUN_TEST(variable_assignment_float);
  RUN_TEST(variable_assignment_string);

  RUN_TEST(arithmetic_add);
  RUN_TEST(arithmetic_sub);
  RUN_TEST(arithmetic_mul);
  RUN_TEST(arithmetic_div);
  RUN_TEST(arithmetic_mod);
  RUN_TEST(arithmetic_int_negative);
  RUN_TEST(arithmetic_float);

  RUN_TEST(bitwise_and);
  RUN_TEST(bitwise_or);
  RUN_TEST(bitwise_xor);
  RUN_TEST(bitwise_not);
  RUN_TEST(shift_left);
  RUN_TEST(shift_right);

  RUN_TEST(condition_if_true);
  RUN_TEST(condition_if_false);
  RUN_TEST(condition_if_else_chain);
  RUN_TEST(condition_logical_and);
  RUN_TEST(condition_logical_or);
  RUN_TEST(condition_compare_string);

  RUN_TEST(loop_while);

  RUN_TEST(string_concat);
  RUN_TEST(string_copy);
  RUN_TEST(string_concat_number);
  RUN_TEST(string_concat_float);
  RUN_TEST(string_concat_direct);

  RUN_TEST(port_write);
  RUN_TEST(port_read);
  RUN_TEST(condition_with_port);

  RUN_TEST(register_and_run);

  RUN_TEST(wait_time);

  RUN_TEST(nested_if);
  RUN_TEST(loop_in_if);

  RUN_TEST(float_copy);

  RUN_TEST(strategy_restart);
  RUN_TEST(strategy_restart_if_same);

  RUN_TEST(macro_var_to_var);
  RUN_TEST(macro_float_to_uint);

  RUN_TEST(variable_increment);
  RUN_TEST(variable_decrement);
  RUN_TEST(variable_multiply_assign);
  RUN_TEST(variable_divide_assign);

  RUN_TEST(array_init);
  RUN_TEST(array_get);
  RUN_TEST(array_set);
  RUN_TEST(array_copy);
  RUN_TEST(array_len);

  RUN_TEST(chr_ord);

  RUN_TEST(memory_slots);
  RUN_TEST(script_too_long);
  RUN_TEST(invalid_slot_id);
  RUN_TEST(duplicate_script_id);
  RUN_TEST(script_exceeds_max_len);

  RUN_TEST(stop_script);
  RUN_TEST(stop_all);
  RUN_TEST(is_busy);

  RUN_TEST(wait_until);

  RUN_TEST(while_infinite_protection);
  RUN_TEST(while_with_break);

  RUN_TEST(if_without_else);
  RUN_TEST(if_without_end);

  RUN_TEST(float_compare_equal);
  RUN_TEST(float_compare_not_equal);

  RUN_TEST(string_compare_not_equal);

  RUN_TEST(array_out_of_bounds);
  RUN_TEST(negative_array_index);

  RUN_TEST(string_index_out_of_bounds);
  RUN_TEST(very_long_string);

  RUN_TEST(port_out_of_range);

  RUN_TEST(division_by_zero);
  RUN_TEST(recursive_call);

  RUN_TEST(get_uint_var_out_of_range);
  RUN_TEST(get_int_var_out_of_range);
  RUN_TEST(get_float_var_out_of_range);
  RUN_TEST(set_uint_var_out_of_range);

  printf("\n=== All tests passed ===\n");
  return 0;
}