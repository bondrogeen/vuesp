#include "./BytecodeFormat.h"

namespace vuesp {
namespace bytecode {

uint32_t crc32(const uint8_t* data, uint16_t size) {
  uint32_t crc = 0xffffffffu;
  for (uint16_t i = 0; i < size; i++) {
    crc ^= data[i];
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 1u) ? ((crc >> 1) ^ 0xedb88320u) : (crc >> 1);
    }
  }
  return crc ^ 0xffffffffu;
}

uint32_t crc32Container(const uint8_t* data, uint16_t size) {
  if (size < kHeaderSize) return crc32(data, size);
  uint32_t crc = 0xffffffffu;
  const uint8_t* zero = reinterpret_cast<const uint8_t*>("\0\0\0\0");
  for (uint16_t i = 0; i < size; i++) {
    const uint8_t byte = (i >= 24 && i < 28) ? zero[i - 24] : data[i];
    crc ^= byte;
    for (uint8_t bit = 0; bit < 8; bit++) {
      crc = (crc & 1u) ? ((crc >> 1) ^ 0xedb88320u) : (crc >> 1);
    }
  }
  return crc ^ 0xffffffffu;
}

static bool validConstantTag(uint8_t tag) {
  return tag >= (uint8_t)ConstantTag::INT32 && tag <= (uint8_t)ConstantTag::ARRAY;
}

bool parseContainer(const uint8_t* data, uint16_t size, ContainerView& out, ParseError& error) {
  error = ParseError::NONE;
  out.bytes = data;
  out.size = size;
  out.constants = nullptr;
  out.constantsSize = 0;
  out.code = nullptr;
  out.blocks = nullptr;
  out.handlers = nullptr;

  if (!data || size < kHeaderSize) {
    error = ParseError::SHORT;
    return false;
  }
  if (data[0] != 'V' || data[1] != 'B' || data[2] != 'C' || data[3] != '1') {
    error = ParseError::MAGIC;
    return false;
  }
  if (data[4] != kVersion) {
    error = ParseError::VERSION;
    return false;
  }

  Header& header = out.header;
  header.version = data[4];
  header.flags = data[5];
  header.headerSize = readU16(data + 6);
  header.codeLength = readU16(data + 8);
  header.constCount = readU16(data + 10);
  header.blockCount = readU16(data + 12);
  header.handlerCount = readU16(data + 14);
  header.entryOffset = readU16(data + 16);
  header.stackMax = readU16(data + 18);
  header.sourceHash = readU32(data + 20);
  header.crc32 = readU32(data + 24);

  if (header.headerSize != kHeaderSize) {
    error = ParseError::HEADER_SIZE;
    return false;
  }
  if (header.codeLength == 0 || header.codeLength > kMaxCodeBytes) {
    error = ParseError::CODE_SIZE;
    return false;
  }
  if (header.stackMax > kStackSize) {
    error = ParseError::STACK_SIZE;
    return false;
  }
  if (header.entryOffset >= header.codeLength) {
    error = ParseError::BOUNDS;
    return false;
  }
  if (crc32Container(data, size) != header.crc32) {
    error = ParseError::CRC;
    return false;
  }

  uint16_t offset = kHeaderSize;
  const uint16_t constantsStart = offset;
  for (uint16_t i = 0; i < header.constCount; i++) {
    if ((uint32_t)offset + 3u > size) {
      error = ParseError::BOUNDS;
      return false;
    }
    const uint8_t tag = data[offset];
    const uint16_t len = readU16(data + offset + 1);
    offset = (uint16_t)(offset + 3);
    if (!validConstantTag(tag) || (uint32_t)offset + len > size) {
      error = ParseError::CONSTANT;
      return false;
    }
    if ((tag == (uint8_t)ConstantTag::INT32 || tag == (uint8_t)ConstantTag::UINT32 ||
         tag == (uint8_t)ConstantTag::FLOAT32) &&
        len != 4) {
      error = ParseError::CONSTANT;
      return false;
    }
    offset = (uint16_t)(offset + len);
  }
  out.constants = data + constantsStart;
  out.constantsSize = (uint16_t)(offset - constantsStart);

  const uint32_t codeEnd = (uint32_t)offset + header.codeLength;
  if (codeEnd > size) {
    error = ParseError::BOUNDS;
    return false;
  }
  out.code = data + offset;
  offset = (uint16_t)codeEnd;

  const uint32_t blocksEnd = (uint32_t)offset + (uint32_t)header.blockCount * 7u;
  if (blocksEnd > size) {
    error = ParseError::BOUNDS;
    return false;
  }
  out.blocks = data + offset;
  offset = (uint16_t)blocksEnd;

  const uint32_t handlersEnd = (uint32_t)offset + (uint32_t)header.handlerCount * 7u;
  if (handlersEnd != size) {
    error = handlersEnd > size ? ParseError::BOUNDS : ParseError::TRAILING;
    return false;
  }
  out.handlers = data + offset;
  return true;
}

bool getConstant(const ContainerView& view, uint16_t index, ConstantRef& out) {
  uint16_t offset = 0;
  for (uint16_t i = 0; i < view.header.constCount; i++) {
    if ((uint32_t)offset + 3u > view.constantsSize) return false;
    const uint8_t tag = view.constants[offset];
    const uint16_t len = readU16(view.constants + offset + 1);
    offset = (uint16_t)(offset + 3);
    if ((uint32_t)offset + len > view.constantsSize) return false;
    if (i == index) {
      out.tag = (ConstantTag)tag;
      out.data = view.constants + offset;
      out.len = len;
      return true;
    }
    offset = (uint16_t)(offset + len);
  }
  return false;
}

bool getBlock(const ContainerView& view, uint16_t index, BlockRef& out) {
  if (index >= view.header.blockCount || !view.blocks) return false;
  const uint8_t* p = view.blocks + (uint32_t)index * 7u;
  out.kind = (BlockKind)p[0];
  out.start = readU16(p + 1);
  out.middle = readU16(p + 3);
  out.end = readU16(p + 5);
  return true;
}

bool getHandler(const ContainerView& view, uint16_t index, HandlerRef& out) {
  if (index >= view.header.handlerCount || !view.handlers) return false;
  const uint8_t* p = view.handlers + (uint32_t)index * 7u;
  out.eventId = p[0];
  out.nameConst = readU16(p + 1);
  out.entry = readU16(p + 3);
  out.end = readU16(p + 5);
  return true;
}

const char* parseErrorName(ParseError error) {
  switch (error) {
    case ParseError::NONE:
      return "none";
    case ParseError::SHORT:
      return "short container";
    case ParseError::MAGIC:
      return "bad magic";
    case ParseError::VERSION:
      return "unsupported version";
    case ParseError::HEADER_SIZE:
      return "unsupported header size";
    case ParseError::CRC:
      return "crc mismatch";
    case ParseError::BOUNDS:
      return "section out of bounds";
    case ParseError::CONSTANT:
      return "invalid constant";
    case ParseError::TRAILING:
      return "trailing bytes";
    case ParseError::CODE_SIZE:
      return "code size out of range";
    case ParseError::STACK_SIZE:
      return "stack size out of range";
    default:
      return "unknown";
  }
}

}  // namespace bytecode
}  // namespace vuesp
