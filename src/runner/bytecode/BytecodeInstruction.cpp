#include "./BytecodeInstruction.h"

namespace vuesp {
namespace bytecode {

bool decodeInstruction(const uint8_t* code, uint16_t codeLength, uint16_t offset,
                       DecodedInstruction& out) {
  if (!code || offset >= codeLength) return false;
  const Opcode op = (Opcode)code[offset];
  const OpcodeInfo* info = opcodeInfo(op);
  if (!info) return false;

  out.op = op;
  out.size = 0;
  out.operandCount = info->operandCount;
  out.operands[0] = 0;
  out.operands[1] = 0;

  uint16_t cursor = (uint16_t)(offset + 1);
  for (uint8_t i = 0; i < info->operandCount; i++) {
    const uint8_t sizeCode = (uint8_t)((info->operandSizes >> (i * 2)) & 0x03);
    const uint8_t width = sizeCode == 0 ? 1 : (sizeCode == 1 ? 2 : 4);
    if ((uint32_t)cursor + width > codeLength) return false;
    uint32_t value = 0;
    for (uint8_t j = 0; j < width; j++) {
      value |= ((uint32_t)code[cursor + j]) << (8 * j);
    }
    out.operands[i] = value;
    cursor = (uint16_t)(cursor + width);
  }
  out.size = (uint8_t)(cursor - offset);
  return true;
}

}  // namespace bytecode
}  // namespace vuesp
