#ifndef VUESP_RUNNER_TYPES_H
#define VUESP_RUNNER_TYPES_H

#include <stdint.h>

// Shared value/context types and provider contracts used by both the bytecode
// VM and the firmware integrations (GPIO, device data, discovery, Modbus).

#define MAX_UINT_VARS 10
#define MAX_INT_VARS 10
#define MAX_FLOAT_VARS 5
#define MAX_STRING_VARS 2
#define MAX_ARRAY_VARS 5
#define MAX_ARRAY_SIZE 64
#define MAX_STRING_LEN 32

#define MAX_PWM_VALUE 255

#define MAX_EXTERNAL_FUNCTIONS 16
#define MAX_FUNCTION_PARAMS 5
#define MAX_CALL_PARAMS 4
#define MAX_EVENT_PARAMS 4

#define PORT_READ 0
#define PORT_WRITE 1

enum DataKind : uint8_t {
  KIND_UINT,
  KIND_INT,
  KIND_FLOAT,
  KIND_STRING
};

enum ValueType : uint8_t {
  VAL_NONE = 0,
  VAL_INT,
  VAL_UINT,
  VAL_FLOAT,
  VAL_STRING,
  VAL_ARRAY
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

struct ScriptContext {
  uint32_t uintVars[MAX_UINT_VARS];
  int32_t intVars[MAX_INT_VARS];
  double floatVars[MAX_FLOAT_VARS];
  char stringVars[MAX_STRING_VARS][MAX_STRING_LEN];
  uint8_t arrayVars[MAX_ARRAY_VARS][MAX_ARRAY_SIZE];
  uint8_t arrayLen[MAX_ARRAY_VARS];

  int32_t eventParams[MAX_EVENT_PARAMS];
  uint8_t eventParamCount;
};

struct Value {
  ValueType type;
  union {
    int32_t intVal;
    uint32_t uintVal;
    float floatVal;
    struct {
      const char* data;
      uint8_t len;
    } stringVal;
    struct {
      const uint8_t* data;
      uint8_t len;
    } arrayVal;
  };
};

typedef bool (*ExternalFunction)(
    uint8_t paramCount,
    const Value* params,
    Value& result,
    void* userData);

typedef bool (*DataProvider)(const char* id, DataKind kind, DataValue& value, bool write);
typedef void (*LogProvider)(const char* message);
typedef bool (*PortProvider)(uint8_t gpio, uint8_t action, uint16_t& value);
typedef void (*StateChangeProvider)(uint8_t gpio, uint16_t newValue);
typedef void (*ScriptCompleteCallback)(uint8_t slot, uint8_t id);

#endif
