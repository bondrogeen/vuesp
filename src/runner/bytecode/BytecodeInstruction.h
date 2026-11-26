#ifndef VUESP_BYTECODE_INSTRUCTION_H
#define VUESP_BYTECODE_INSTRUCTION_H

#include <stdint.h>

#include "./Opcode.h"

namespace vuesp {
namespace bytecode {

struct DecodedInstruction {
  Opcode op;
  uint8_t size;
  uint8_t operandCount;
  uint32_t operands[2];
};

bool decodeInstruction(const uint8_t* code, uint16_t codeLength, uint16_t offset,
                       DecodedInstruction& out);

}  // namespace bytecode
}  // namespace vuesp

#endif
