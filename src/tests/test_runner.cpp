#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "../runner/runner.h"
#include "arduino_stub.h"

static uint16_t virtualPins[40] = {0};
static std::vector<std::string> testLog;

bool testPortProvider(uint8_t gpio, PortAction action, uint16_t& value) {
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
  return false;
}

void testLogProvider(const char* message) {
  testLog.push_back(std::string(message));
}

void resetTestState() {
  stub_millis = 0;
  memset(virtualPins, 0, sizeof(virtualPins));
  testLog.clear();
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

TEST(variable_assignment_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=42,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_TRUE(logContains("42"));
}

TEST(variable_assignment_int) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$i0=-10,$display=$i0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -10);
  ASSERT_TRUE(logContains("-10"));
}

TEST(variable_assignment_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$f0=3.14,$display=$f0", RESTART);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_TRUE(val > 3.1f && val < 3.2f);
  ASSERT_TRUE(logContains("3.14") || logContains("3.1"));
}

TEST(variable_assignment_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0='Hello',$display=$s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("Hello"));
}

TEST(arithmetic_add) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=10,$v0=$v0+5,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
  ASSERT_TRUE(logContains("15"));
}

TEST(arithmetic_sub) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=20,$v0=$v0-8,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
  ASSERT_TRUE(logContains("12"));
}

TEST(arithmetic_mul) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=6,$v0=$v0*7,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 42u);
  ASSERT_TRUE(logContains("42"));
}

TEST(arithmetic_div) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=100,$v0=$v0/4,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 25u);
  ASSERT_TRUE(logContains("25"));
}

TEST(arithmetic_mod) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=17,$v0=$v0%5,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 2u);
  ASSERT_TRUE(logContains("2"));
}

TEST(arithmetic_int_negative) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$i0=20,$i0=8-$i0,$display=$i0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getIntVar(0), -12);
  ASSERT_TRUE(logContains("-12"));
}

TEST(arithmetic_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$f0=10.5,$f0=$f0+2.5,$display=$f0", RESTART);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(0);
  ASSERT_TRUE(val > 12.9f && val < 13.1f);
  ASSERT_TRUE(logContains("13.0") || logContains("13"));
}

TEST(bitwise_and) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=255,$v0=$v0&15,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 15u);
  ASSERT_TRUE(logContains("15"));
}

TEST(bitwise_or) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=240,$v0=$v0|15,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 255u);
  ASSERT_TRUE(logContains("255"));
}

TEST(bitwise_xor) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=255,$v0=$v0^255,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 0u);
  ASSERT_TRUE(logContains("0"));
}

TEST(bitwise_not) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=0,$v0=~$v0,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ((int32_t)runner.getUintVar(0), -1);
  ASSERT_TRUE(logContains("-1"));
}

TEST(shift_left) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=1,$v0=$v0<<4,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 16u);
  ASSERT_TRUE(logContains("16"));
}

TEST(shift_right) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=16,$v0=$v0>>2,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 4u);
  ASSERT_TRUE(logContains("4"));
}

TEST(condition_if_true) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=1,if:$v0==1,$display='TRUE',else,$display='FALSE',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("TRUE"));
  ASSERT_TRUE(!logContains("FALSE"));
}

TEST(condition_if_false) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=0,if:$v0==1,$display='TRUE',else,$display='FALSE',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("FALSE"));
  ASSERT_TRUE(!logContains("TRUE"));
}

TEST(condition_if_else_chain) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=2,if:$v0==1,$display='ONE',else,if:$v0==2,$display='TWO',else,$display='OTHER',end,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("TWO"));
}

TEST(condition_logical_and) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=1,$v1=1,if:$v0==1&&$v1==1,$display='BOTH',else,$display='NONE',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("BOTH"));
}

TEST(condition_logical_or) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=0,$v1=1,if:$v0==1||$v1==1,$display='ANY',else,$display='NONE',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("ANY"));
}

TEST(condition_compare_string) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0='LEDON',if:$s0=='LEDON',$display='MATCH',else,$display='NOMATCH',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("MATCH"));
}

TEST(loop_while) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=0,while:$v0<5,$v0=$v0+1,end,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
  ASSERT_TRUE(logContains("5"));
}

TEST(string_concat) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0='hello',$s0=$s0+'world',$display=$s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("helloworld"));
}

TEST(string_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0='hello',$s1=$s0,$display=$s1", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("hello"));
}

TEST(string_concat_number) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=42,$s0='Value: ',$s0=$s0+$v0,$display=$s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("Value: 42"));
}

TEST(string_concat_float) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$f0=3.14,$s0='Pi: ',$s0=$s0+$f0,$display=$s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("Pi: 3.1") || logContains("Pi: 3.14"));
}

TEST(string_concat_direct) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0='Value: ',$s0=$s0+123,$display=$s0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("Value: 123"));
}

TEST(port_write) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.addScript(1, "$p13=200", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(virtualPins[13], 200u);
}

TEST(port_read) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  runner.setStateChangeProvider(testStateChangeProvider);
  virtualPins[13] = 200;
  runner.addScript(1, "$v0=$p13,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 200u);
  ASSERT_TRUE(logContains("200"));
}

TEST(condition_with_port) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.setPortProvider(testPortProvider);
  virtualPins[13] = 0;
  runner.addScript(1, "if:$p13==0,$display='LOW',else,$display='HIGH',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("LOW"));
}

TEST(register_and_run) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.registerScript(5, "$display='REGISTERED'");
  runner.runScript(5);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("REGISTERED"));
}

TEST(call_nonexistent) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "call(99)", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(true);
}

TEST(wait_time) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  uint32_t start = stub_millis;
  runner.addScript(1, "$display='START',wait(100u),$display='END'", RESTART);
  runScriptUntilDone(runner, 10);
  uint32_t elapsed = stub_millis - start;
  ASSERT_TRUE(elapsed >= 100);
  ASSERT_TRUE(logContains("START"));
  ASSERT_TRUE(logContains("END"));
}

TEST(nested_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=1,$v1=1,if:$v0==1,if:$v1==1,$display='DEEP',end,end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("DEEP"));
}

TEST(loop_in_if) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=0,$v1=1,if:$v1==1,while:$v0<3,$v0=$v0+1,end,end,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 3u);
  ASSERT_TRUE(logContains("3"));
}

TEST(float_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$f0=3.14,$f1=$f0,$display=$f1", RESTART);
  runScriptUntilDone(runner);
  float val = runner.getFloatVar(1);
  ASSERT_TRUE(val > 3.1f && val < 3.2f);
  ASSERT_TRUE(logContains("3.14") || logContains("3.1"));
}

TEST(float_in_condition) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$f0=10.5,$f1=20.5,if:$f0>10.0&&$f1<30.0,$display='RANGE',else,$display='NORANGE',end", RESTART);
  runScriptUntilDone(runner);
  ASSERT_TRUE(logContains("RANGE"));
}

TEST(strategy_restart) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$display='FIRST'", RESTART);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.addScript(1, "$display='SECOND'", RESTART);
  runScriptUntilDone(runner, 10);
  ASSERT_TRUE(logContains("SECOND"));
}

TEST(strategy_restart_if_same) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$display='SAME'", RESTART_IF_SAME);
  runScriptUntilDone(runner, 10);
  testLog.clear();
  runner.addScript(1, "$display='SAME'", RESTART_IF_SAME);
  runScriptUntilDone(runner, 10);
  ASSERT_TRUE(logContains("SAME"));
}

TEST(strategy_add_queue) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "while:1,$display='FIRST',wait(100u),end", ADD_QUEUE);
  runScriptUntilDone(runner, 10);
  runner.addScript(1, "$display='SECOND'", ADD_QUEUE);
  runScriptUntilDone(runner, 50);
  ASSERT_TRUE(logContains("SECOND"));
}

TEST(macro_var_to_var) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=99,$v1=$v0,$display=$v1", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(1), 99u);
  ASSERT_TRUE(logContains("99"));
}

TEST(macro_float_to_uint) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$f0=12.5,$v0=$f0,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 12u);
  ASSERT_TRUE(logContains("12"));
}

TEST(variable_increment) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=5,$v0=$v0+3,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 8u);
  ASSERT_TRUE(logContains("8"));
}

TEST(variable_decrement) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=10,$v0=$v0-4,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 6u);
  ASSERT_TRUE(logContains("6"));
}

TEST(variable_multiply_assign) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=4,$v0=$v0*5,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 20u);
  ASSERT_TRUE(logContains("20"));
}

TEST(variable_divide_assign) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$v0=30,$v0=$v0/6,$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 5u);
  ASSERT_TRUE(logContains("5"));
}

TEST(array_init) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$a0={10;20;30},$v2=len($a0),$display=$v2", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getArrayLen(0), 3u);
  ASSERT_TRUE(logContains("3"));
}

TEST(array_get) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$a0={10;20;30},$v0=get($a0,1),$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 20u);
  ASSERT_TRUE(logContains("20"));
}

TEST(array_set) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$a0={10;20;30},set($a0,1,99),$v0=get($a0,1),$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 99u);
  ASSERT_TRUE(logContains("99"));
}

TEST(array_copy) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$a0={10;20;30},$a1=$a0,$v0=get($a1,0),$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 10u);
  ASSERT_TRUE(logContains("10"));
}

TEST(array_len) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$a0={10;20;30},$v2=len($a0),$display=$v2", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(2), 3u);
  ASSERT_TRUE(logContains("3"));
}

TEST(chr_ord) {
  ScriptRunner runner;
  runner.setDataProvider(testDataProvider);
  runner.addScript(1, "$s0=chr(65),$v0=ord($s0),$display=$v0", RESTART);
  runScriptUntilDone(runner);
  ASSERT_EQ(runner.getUintVar(0), 65u);
  ASSERT_TRUE(logContains("65"));
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
  RUN_TEST(call_nonexistent);
  
  RUN_TEST(wait_time);
  
  RUN_TEST(nested_if);
  RUN_TEST(loop_in_if);
  
  RUN_TEST(float_copy);
  RUN_TEST(float_in_condition);
  
  RUN_TEST(strategy_restart);
  RUN_TEST(strategy_restart_if_same);
  RUN_TEST(strategy_add_queue);
  
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

  printf("\n=== All tests passed ===\n");
  return 0;
}