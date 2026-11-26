#include "./BytecodeVerifier.h"

#include <string.h>

#include "./BytecodeInstruction.h"
#include "./Opcode.h"

namespace vuesp {
namespace bytecode {

namespace {

uint8_t g_boundaries[(kMaxCodeBytes + 7) / 8];
uint8_t g_depth[kMaxCodeBytes];

inline bool isBoundary(uint16_t offset) {
  return (g_boundaries[offset >> 3] & (uint8_t)(1u << (offset & 7))) != 0;
}

inline void markBoundary(uint16_t offset) {
  g_boundaries[offset >> 3] |= (uint8_t)(1u << (offset & 7));
}

bool fail(VerifyResult& result, VerifyError error, uint16_t offset) {
  result.ok = false;
  result.error = error;
  result.offset = offset;
  return false;
}

const FunctionSpec* findFunction(const Capabilities& caps, uint8_t id) {
  for (uint8_t i = 0; i < caps.functionCount; i++) {
    if (caps.functions[i].id == id) return &caps.functions[i];
  }
  return nullptr;
}

const EventSpec* findEvent(const Capabilities& caps, uint8_t id) {
  for (uint8_t i = 0; i < caps.eventCount; i++) {
    if (caps.events[i].id == id) return &caps.events[i];
  }
  return nullptr;
}

bool constantOf(const ContainerView& view, uint16_t index, ConstantTag tag, uint16_t maxLen) {
  ConstantRef ref;
  if (!getConstant(view, index, ref)) return false;
  if (ref.tag != tag) {
    if (tag == ConstantTag::NAME && ref.tag == ConstantTag::STRING) return true;
    if (tag == ConstantTag::STRING && ref.tag == ConstantTag::NAME) return true;
    return false;
  }
  if (maxLen && ref.len > maxLen) return false;
  return true;
}

bool stackEffect(const DecodedInstruction& inst, const Capabilities& caps, uint8_t& pop,
                 uint8_t& push, bool& terminal) {
  pop = 0;
  push = 0;
  terminal = false;
  switch (inst.op) {
    case Opcode::PUSH_I8:
    case Opcode::PUSH_I16:
    case Opcode::PUSH_I32:
    case Opcode::PUSH_U8:
    case Opcode::PUSH_U16:
    case Opcode::PUSH_U32:
    case Opcode::PUSH_F32:
    case Opcode::PUSH_STR:
    case Opcode::PUSH_ARRAY:
    case Opcode::LOAD_VAR:
    case Opcode::LOAD_DATA:
      push = 1;
      return true;
    case Opcode::STORE_VAR:
    case Opcode::STORE_DATA:
      pop = 1;
      return true;
    case Opcode::DUP:
      pop = 1;
      push = 2;
      return true;
    case Opcode::POP:
      pop = 1;
      return true;
    case Opcode::SWAP:
      pop = 2;
      push = 2;
      return true;
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
      pop = 2;
      push = 1;
      return true;
    case Opcode::NEG:
    case Opcode::NOT:
    case Opcode::BIT_NOT:
    case Opcode::LEN:
    case Opcode::CHR:
    case Opcode::ORD:
    case Opcode::ARRAY_LEN:
      pop = 1;
      push = 1;
      return true;
    case Opcode::ARRAY_GET:
    case Opcode::GET:
      pop = 2;
      push = 1;
      return true;
    case Opcode::ARRAY_SET:
    case Opcode::SET:
      pop = 3;
      return true;
    case Opcode::JUMP:
      return true;
    case Opcode::JUMP_IF_FALSE:
    case Opcode::JUMP_IF_TRUE:
      pop = 1;
      return true;
    case Opcode::WAIT:
      return true;
    case Opcode::HALT:
    case Opcode::RETURN:
      terminal = true;
      return true;
    case Opcode::LOG:
      pop = (uint8_t)inst.operands[0];
      return true;
    case Opcode::CALL_SCRIPT:
      pop = (uint8_t)inst.operands[1];
      return true;
    case Opcode::CALL_SCRIPT_NAME:
      pop = (uint8_t)inst.operands[1];
      return true;
    case Opcode::CALL_EXT: {
      const FunctionSpec* fn = findFunction(caps, (uint8_t)inst.operands[0]);
      if (!fn) return false;
      pop = (uint8_t)inst.operands[1];
      push = fn->ret == VALUE_NONE ? 0 : 1;
      return true;
    }
    default:
      return false;
  }
}

bool propagate(uint16_t target, uint8_t depth, uint16_t codeLength, VerifyResult& result) {
  if (target >= codeLength || !isBoundary(target)) {
    fail(result, VerifyError::JUMP, target);
    return false;
  }
  if (g_depth[target] == 0xFF) {
    g_depth[target] = depth;
    return true;
  }
  if (g_depth[target] != depth) {
    fail(result, VerifyError::STACK, target);
    return false;
  }
  return false;
}

bool seed(uint16_t target, uint16_t codeLength, VerifyResult& result) {
  if (target >= codeLength || !isBoundary(target)) {
    fail(result, VerifyError::BOUNDS, target);
    return false;
  }
  if (g_depth[target] == 0xFF) {
    g_depth[target] = 0;
    return true;
  }
  if (g_depth[target] != 0) {
    fail(result, VerifyError::STACK, target);
    return false;
  }
  return true;
}

}  // namespace

bool verifyContainer(const ContainerView& view, const Capabilities& caps, VerifyResult& result) {
  result.ok = true;
  result.error = VerifyError::NONE;
  result.offset = 0;

  const uint16_t codeLength = view.header.codeLength;
  const uint8_t* code = view.code;
  if (!code || codeLength == 0 || codeLength > kMaxCodeBytes) {
    return fail(result, VerifyError::BOUNDS, 0);
  }
  if (codeLength > caps.maxCodeBytes) {
    return fail(result, VerifyError::BOUNDS, 0);
  }
  if (view.header.stackMax > caps.maxStack || view.header.stackMax > kStackSize) {
    return fail(result, VerifyError::STACK, 0);
  }

  memset(g_boundaries, 0, sizeof(g_boundaries));
  uint16_t offset = 0;
  while (offset < codeLength) {
    markBoundary(offset);
    DecodedInstruction inst;
    if (!decodeInstruction(code, codeLength, offset, inst)) {
      return fail(result, VerifyError::OPCODE, offset);
    }
    offset = (uint16_t)(offset + inst.size);
  }
  if (offset != codeLength) {
    return fail(result, VerifyError::OPCODE, offset);
  }

  // Operand validation.
  offset = 0;
  while (offset < codeLength) {
    DecodedInstruction inst;
    if (!decodeInstruction(code, codeLength, offset, inst)) {
      return fail(result, VerifyError::OPCODE, offset);
    }

    switch (inst.op) {
      case Opcode::PUSH_STR:
        if (!constantOf(view, (uint16_t)inst.operands[0], ConstantTag::STRING, 0)) {
          return fail(result, VerifyError::CONST, offset);
        }
        break;
      case Opcode::PUSH_ARRAY:
        if (!constantOf(view, (uint16_t)inst.operands[0], ConstantTag::ARRAY, caps.maxArrayLen)) {
          return fail(result, VerifyError::CONST, offset);
        }
        break;
      case Opcode::LOAD_DATA:
        if (!constantOf(view, (uint16_t)inst.operands[0], ConstantTag::NAME, 0) ||
            inst.operands[1] > 3) {
          return fail(result, VerifyError::CONST, offset);
        }
        break;
      case Opcode::STORE_DATA:
        if (!constantOf(view, (uint16_t)inst.operands[0], ConstantTag::NAME, 0)) {
          return fail(result, VerifyError::CONST, offset);
        }
        break;
      case Opcode::LOAD_VAR:
      case Opcode::STORE_VAR: {
        const uint8_t varid = (uint8_t)inst.operands[0];
        const uint8_t type = (uint8_t)((varid >> 4) & 0x0F);
        const uint8_t index = (uint8_t)(varid & 0x0F);
        if (type >= 8 || index >= caps.vars[type]) {
          return fail(result, VerifyError::VAR, offset);
        }
        if (inst.op == Opcode::STORE_VAR && type >= 6) {
          return fail(result, VerifyError::VAR, offset);
        }
        break;
      }
      case Opcode::CALL_SCRIPT:
        if (inst.operands[0] < 1 || inst.operands[0] > 254 || inst.operands[1] > 4) {
          return fail(result, VerifyError::FUNC, offset);
        }
        break;
      case Opcode::CALL_SCRIPT_NAME:
        if (!constantOf(view, (uint16_t)inst.operands[0], ConstantTag::NAME,
                        kMaxScriptNameLen) ||
            inst.operands[1] > 4) {
          return fail(result, VerifyError::CONST, offset);
        }
        break;
      case Opcode::CALL_EXT: {
        const FunctionSpec* fn = findFunction(caps, (uint8_t)inst.operands[0]);
        if (!fn || inst.operands[1] < fn->minArgs || inst.operands[1] > fn->maxArgs) {
          return fail(result, VerifyError::FUNC, offset);
        }
        break;
      }
      case Opcode::LOG:
        if (inst.operands[0] > 8) {
          return fail(result, VerifyError::LIMIT, offset);
        }
        break;
      case Opcode::JUMP:
      case Opcode::JUMP_IF_FALSE:
      case Opcode::JUMP_IF_TRUE:
        if (inst.operands[0] >= codeLength || !isBoundary((uint16_t)inst.operands[0])) {
          return fail(result, VerifyError::JUMP, offset);
        }
        break;
      default:
        break;
    }
    offset = (uint16_t)(offset + inst.size);
  }

  // Handler table.
  for (uint16_t i = 0; i < view.header.handlerCount; i++) {
    HandlerRef handler;
    if (!getHandler(view, i, handler)) {
      return fail(result, VerifyError::BOUNDS, 0);
    }
    if (!findEvent(caps, handler.eventId)) {
      return fail(result, VerifyError::EVENT, handler.entry);
    }
    if (!constantOf(view, handler.nameConst, ConstantTag::STRING, 0)) {
      return fail(result, VerifyError::EVENT, handler.entry);
    }
    if (handler.entry >= handler.end || handler.end > codeLength ||
        !isBoundary(handler.entry) ||
        (handler.end < codeLength && !isBoundary(handler.end))) {
      return fail(result, VerifyError::EVENT, handler.entry);
    }
    if (handler.end == 0 || code[handler.end - 1] != (uint8_t)Opcode::RETURN) {
      return fail(result, VerifyError::EVENT, handler.entry);
    }
  }

  // Block table.
  for (uint16_t i = 0; i < view.header.blockCount; i++) {
    BlockRef block;
    if (!getBlock(view, i, block)) {
      return fail(result, VerifyError::BLOCK, 0);
    }
    const uint8_t kind = (uint8_t)block.kind;
    if (kind < 1 || kind > 5) {
      return fail(result, VerifyError::BLOCK, block.start);
    }
    if (block.start > block.end || block.start >= codeLength ||
        (block.end < codeLength && !isBoundary(block.end)) || !isBoundary(block.start)) {
      return fail(result, VerifyError::BLOCK, block.start);
    }
    if (kind == 1 || kind == 3 || kind == 5) {
      if (block.middle < block.start || block.middle >= block.end ||
          block.middle >= codeLength || !isBoundary(block.middle)) {
        return fail(result, VerifyError::BLOCK, block.start);
      }
      DecodedInstruction inst;
      if (!decodeInstruction(code, codeLength, block.middle, inst) ||
          inst.op != Opcode::JUMP_IF_FALSE) {
        return fail(result, VerifyError::BLOCK, block.middle);
      }
    } else if (block.middle != 0) {
      return fail(result, VerifyError::BLOCK, block.start);
    }
  }

  // Stack depth dataflow.
  memset(g_depth, 0xFF, sizeof(g_depth));
  if (!seed(view.header.entryOffset, codeLength, result)) return false;
  for (uint16_t i = 0; i < view.header.handlerCount; i++) {
    HandlerRef handler;
    if (!getHandler(view, i, handler)) {
      return fail(result, VerifyError::BOUNDS, 0);
    }
    if (!seed(handler.entry, codeLength, result)) return false;
  }

  uint8_t actualMax = 0;
  bool changed = true;
  uint16_t passes = 0;
  while (changed) {
    if (passes++ > codeLength) {
      return fail(result, VerifyError::LIMIT, 0);
    }
    changed = false;
    offset = 0;
    while (offset < codeLength) {
      DecodedInstruction inst;
      if (!decodeInstruction(code, codeLength, offset, inst)) {
        return fail(result, VerifyError::OPCODE, offset);
      }
      if (g_depth[offset] == 0xFF) {
        offset = (uint16_t)(offset + inst.size);
        continue;
      }

      uint8_t pop = 0;
      uint8_t push = 0;
      bool terminal = false;
      if (!stackEffect(inst, caps, pop, push, terminal)) {
        return fail(result, VerifyError::FUNC, offset);
      }
      const uint8_t depth = g_depth[offset];
      if (depth < pop) {
        return fail(result, VerifyError::STACK, offset);
      }
      const uint8_t nextDepth = (uint8_t)(depth - pop + push);
      if (nextDepth > caps.maxStack) {
        return fail(result, VerifyError::STACK, offset);
      }
      if (nextDepth > actualMax) actualMax = nextDepth;

      if (terminal) {
        if (nextDepth != 0) {
          return fail(result, VerifyError::STACK, offset);
        }
        offset = (uint16_t)(offset + inst.size);
        continue;
      }

      if (inst.op == Opcode::JUMP) {
        const bool now = propagate((uint16_t)inst.operands[0], nextDepth, codeLength, result);
        if (!result.ok) return false;
        changed = changed || now;
      } else if (inst.op == Opcode::JUMP_IF_FALSE || inst.op == Opcode::JUMP_IF_TRUE) {
        bool now = propagate((uint16_t)inst.operands[0], nextDepth, codeLength, result);
        if (!result.ok) return false;
        changed = changed || now;
        now = propagate((uint16_t)(offset + inst.size), nextDepth, codeLength, result);
        if (!result.ok) return false;
        changed = changed || now;
      } else {
        const uint16_t fallthrough = (uint16_t)(offset + inst.size);
        if (fallthrough < codeLength) {
          const bool now = propagate(fallthrough, nextDepth, codeLength, result);
          if (!result.ok) return false;
          changed = changed || now;
        } else if (nextDepth != 0) {
          return fail(result, VerifyError::STACK, offset);
        }
      }
      offset = (uint16_t)(offset + inst.size);
    }
  }

  // Unreachable instructions are still decoded and operand-checked above.
  // They are allowed because dead code after return/break is harmless and the
  // compiler may keep it in v1.

  if (view.header.stackMax < actualMax) {
    return fail(result, VerifyError::STACK, 0);
  }

  result.ok = true;
  result.error = VerifyError::NONE;
  return true;
}

const char* verifyErrorName(VerifyError error) {
  switch (error) {
    case VerifyError::NONE:
      return "none";
    case VerifyError::PARSE:
      return "parse error";
    case VerifyError::OPCODE:
      return "invalid opcode";
    case VerifyError::BOUNDS:
      return "out of bounds";
    case VerifyError::JUMP:
      return "invalid jump";
    case VerifyError::STACK:
      return "stack error";
    case VerifyError::CONST:
      return "invalid constant";
    case VerifyError::VAR:
      return "invalid variable";
    case VerifyError::FUNC:
      return "invalid function";
    case VerifyError::EVENT:
      return "invalid event";
    case VerifyError::BLOCK:
      return "invalid block";
    case VerifyError::UNREACHABLE:
      return "unreachable code";
    case VerifyError::LIMIT:
      return "limit exceeded";
    default:
      return "unknown";
  }
}

}  // namespace bytecode
}  // namespace vuesp
