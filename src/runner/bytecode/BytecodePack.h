#ifndef VUESP_BYTECODE_PACK_H
#define VUESP_BYTECODE_PACK_H

#include <stdint.h>

#include "./BytecodeFormat.h"

namespace vuesp {
namespace bytecode {

// Single-file container for all scripts.
// Layout:
//   header (14 bytes)
//   TOC: count * 25 bytes
//   code blobs
static constexpr uint8_t kPackVersion = 1;
static constexpr uint8_t kPackMaxEntries = 15;
static constexpr uint8_t kPackNameLen = 16;
static constexpr uint8_t kPackEntrySize = 25;
static constexpr uint8_t kPackHeaderSize = 14;
static constexpr uint8_t kPackFlagEnabled = 0x01;
static constexpr uint8_t kPackFlagHasSource = 0x02;
static constexpr uint8_t kPackFlagMask =
    (uint8_t)(kPackFlagEnabled | kPackFlagHasSource);

struct PackHeader {
  uint8_t version;
  uint8_t count;
  uint8_t flags;
  uint8_t reserved;
  uint16_t tocSize;
  uint32_t tocCrc;
};

struct PackEntry {
  uint8_t name[kPackNameLen];
  uint8_t flags;
  uint16_t codeOff;
  uint16_t codeLen;
  uint32_t codeCrc;
};

struct PackView {
  const uint8_t* data;
  uint16_t size;
  uint32_t fileSize;
  PackHeader header;
  const uint8_t* toc;
};

enum class PackError : uint8_t {
  NONE = 0,
  SHORT,
  MAGIC,
  VERSION,
  COUNT,
  TOC_SIZE,
  TOC_CRC,
  NAME,
  DUPLICATE,
  BOUNDS,
  CODE_CRC,
  OVERLAP,
  CAPACITY,
  CODE_LEN,
  FLAGS
};

struct PackInputEntry {
  const char* name;
  uint8_t flags;
  const uint8_t* code;
  uint16_t codeLen;
};

// Lowercases ASCII names, validates [a-z_][a-z0-9_-]{0..15} and writes a
// zero-padded 16-byte key. Returns false for empty/too long/invalid names.
bool packNormalizeName(const char* name, uint8_t out[kPackNameLen]);

// Converts a fixed 16-byte key to a null-terminated string. Returns false if
// the key is not lower-case/valid (padding included).
bool packNameToText(const uint8_t name[kPackNameLen], char out[kPackNameLen + 1]);

bool parsePack(const uint8_t* data, uint16_t size, PackView& out, PackError& error);
// Parses the header and TOC from a buffer that may contain only the TOC.
// `fileSize` is the total pack size on disk and is used for code bounds.
// Per-entry code CRC is not checked; use verifyPackCode for a code blob.
bool parsePackToc(const uint8_t* data, uint16_t size, uint32_t fileSize,
                  PackView& out, PackError& error);
bool verifyPackCode(const PackView& view, uint16_t index, const uint8_t* code,
                    uint16_t len);
bool findPackEntry(const PackView& view, const char* name, uint16_t& index);
bool getPackEntry(const PackView& view, uint16_t index, PackEntry& out);
bool getPackCode(const PackView& view, uint16_t index, const uint8_t*& code, uint16_t& len);

// In-memory builder used by tests and by the web-side pack writer. The service
// can use the same format with a streaming writer later.
bool buildPack(const PackInputEntry* entries, uint8_t count, uint8_t* out,
               uint16_t outCapacity, uint16_t& outLen, PackError& error);

const char* packErrorName(PackError error);

}  // namespace bytecode
}  // namespace vuesp

#endif
