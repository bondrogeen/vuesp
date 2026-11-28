#ifndef VUESP_BYTECODE_CAPABILITIES_H
#define VUESP_BYTECODE_CAPABILITIES_H

#include <stdint.h>

namespace vuesp {
namespace bytecode {

// Keep these values aligned with the TypeScript ValueKind numbers.
enum ValueKind : uint8_t {
  VALUE_INT = 0,
  VALUE_UINT = 1,
  VALUE_FLOAT = 2,
  VALUE_STRING = 3,
  VALUE_ARRAY = 4,
  VALUE_NONE = 5
};

struct FunctionSpec {
  uint8_t id;
  const char* name;
  uint8_t minArgs;
  uint8_t maxArgs;
  uint8_t ret;  // ValueKind
  const char* description;
};

struct EventSpec {
  uint8_t id;
  const char* name;
  uint8_t params;
  const char* args;
  const char* description;
};

struct Capabilities {
  uint8_t vmVersion;
  uint16_t maxCodeBytes;
  uint16_t maxStack;
  uint8_t vars[8];  // u,i,f,s,a,p,e,c
  uint8_t maxStringLen;
  uint8_t maxArrayLen;
  uint16_t instrPerTick;
  uint16_t usPerTick;
  uint16_t updateBudgetUs;
  const FunctionSpec* functions;
  uint8_t functionCount;
  const EventSpec* events;
  uint8_t eventCount;
};

}  // namespace bytecode
}  // namespace vuesp

#endif
