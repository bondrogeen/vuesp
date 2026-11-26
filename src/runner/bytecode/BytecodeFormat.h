#ifndef VUESP_BYTECODE_FORMAT_H
#define VUESP_BYTECODE_FORMAT_H

#include <stdint.h>

#ifndef VBC_MAX_CODE_BYTES
#define VBC_MAX_CODE_BYTES 1024
#endif

#ifndef VBC_MAX_CONTEXTS
#define VBC_MAX_CONTEXTS 15
#endif

#ifndef VBC_STACK_SIZE
#define VBC_STACK_SIZE 64
#endif

#ifndef VBC_SCRATCH_BYTES
#define VBC_SCRATCH_BYTES 256
#endif

#ifndef VBC_MAX_STRING_LEN
#define VBC_MAX_STRING_LEN 32
#endif

#ifndef VBC_MAX_ARRAY_LEN
#define VBC_MAX_ARRAY_LEN 64
#endif

#ifndef VBC_MAX_CALL_DEPTH
#define VBC_MAX_CALL_DEPTH 4
#endif

namespace vuesp {
namespace bytecode {

static constexpr uint8_t kHeaderSize = 28;
static constexpr uint8_t kVersion = 1;
static constexpr uint16_t kMaxCodeBytes = VBC_MAX_CODE_BYTES;
static constexpr uint8_t kMaxContexts = VBC_MAX_CONTEXTS;
static constexpr uint16_t kStackSize = VBC_STACK_SIZE;
static constexpr uint16_t kScratchBytes = VBC_SCRATCH_BYTES;
static constexpr uint8_t kMaxStringLen = VBC_MAX_STRING_LEN;
static constexpr uint8_t kMaxArrayLen = VBC_MAX_ARRAY_LEN;
static constexpr uint8_t kMaxCallDepth = VBC_MAX_CALL_DEPTH;
static constexpr uint8_t kMaxScriptNameLen = 16;

enum class ConstantTag : uint8_t {
  INT32 = 1,
  UINT32 = 2,
  FLOAT32 = 3,
  STRING = 4,
  NAME = 5,
  ARRAY = 6
};

enum class BlockKind : uint8_t {
  IF = 1,
  ELSE = 2,
  WHILE = 3,
  HANDLER = 4,
  TERNARY = 5
};

struct Header {
  uint8_t version;
  uint8_t flags;
  uint16_t headerSize;
  uint16_t codeLength;
  uint16_t constCount;
  uint16_t blockCount;
  uint16_t handlerCount;
  uint16_t entryOffset;
  uint16_t stackMax;
  uint32_t sourceHash;
  uint32_t crc32;
};

struct ContainerView {
  const uint8_t* bytes;
  uint16_t size;
  Header header;
  const uint8_t* constants;
  uint16_t constantsSize;
  const uint8_t* code;
  const uint8_t* blocks;
  const uint8_t* handlers;
};

struct ConstantRef {
  ConstantTag tag;
  const uint8_t* data;
  uint16_t len;
};

struct BlockRef {
  BlockKind kind;
  uint16_t start;
  uint16_t middle;
  uint16_t end;
};

struct HandlerRef {
  uint8_t eventId;
  uint16_t nameConst;
  uint16_t entry;
  uint16_t end;
};

enum class ParseError : uint8_t {
  NONE = 0,
  SHORT,
  MAGIC,
  VERSION,
  HEADER_SIZE,
  CRC,
  BOUNDS,
  CONSTANT,
  TRAILING,
  CODE_SIZE,
  STACK_SIZE
};

inline uint16_t readU16(const uint8_t* data) {
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

inline uint32_t readU32(const uint8_t* data) {
  return (uint32_t)data[0] | ((uint32_t)data[1] << 8) | ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

inline int16_t readI16(const uint8_t* data) {
  return (int16_t)readU16(data);
}

inline void writeU16(uint8_t* data, uint16_t value) {
  data[0] = (uint8_t)(value & 0xff);
  data[1] = (uint8_t)((value >> 8) & 0xff);
}

inline void writeU32(uint8_t* data, uint32_t value) {
  data[0] = (uint8_t)(value & 0xff);
  data[1] = (uint8_t)((value >> 8) & 0xff);
  data[2] = (uint8_t)((value >> 16) & 0xff);
  data[3] = (uint8_t)((value >> 24) & 0xff);
}

inline int32_t readI32(const uint8_t* data) {
  return (int32_t)readU32(data);
}

inline float readF32(const uint8_t* data) {
  union {
    uint32_t u;
    float f;
  } value;
  value.u = readU32(data);
  return value.f;
}

uint32_t crc32(const uint8_t* data, uint16_t size);
// Streaming helper: start with 0xffffffff, then finalize with ^0xffffffff.
uint32_t crc32Update(uint32_t crc, const uint8_t* data, uint16_t size);
uint32_t crc32Container(const uint8_t* data, uint16_t size);
bool parseContainer(const uint8_t* data, uint16_t size, ContainerView& out, ParseError& error);
bool getConstant(const ContainerView& view, uint16_t index, ConstantRef& out);
bool getBlock(const ContainerView& view, uint16_t index, BlockRef& out);
bool getHandler(const ContainerView& view, uint16_t index, HandlerRef& out);
const char* parseErrorName(ParseError error);

}  // namespace bytecode
}  // namespace vuesp

#endif
