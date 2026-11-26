#ifndef VUESP_BYTECODE_VERIFIER_H
#define VUESP_BYTECODE_VERIFIER_H

#include <stdint.h>

#include "./BytecodeCapabilities.h"
#include "./BytecodeFormat.h"

namespace vuesp {
namespace bytecode {

enum class VerifyError : uint8_t {
  NONE = 0,
  PARSE,
  OPCODE,
  BOUNDS,
  JUMP,
  STACK,
  CONST,
  VAR,
  FUNC,
  EVENT,
  BLOCK,
  UNREACHABLE,
  LIMIT
};

struct VerifyResult {
  bool ok;
  VerifyError error;
  uint16_t offset;
};

bool verifyContainer(const ContainerView& view, const Capabilities& caps, VerifyResult& result);
const char* verifyErrorName(VerifyError error);

}  // namespace bytecode
}  // namespace vuesp

#endif
