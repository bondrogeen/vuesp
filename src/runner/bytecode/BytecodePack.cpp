#include "./BytecodePack.h"

#include <string.h>

namespace vuesp {
namespace bytecode {

namespace {

const uint8_t kPackMagic[4] = {'V', 'S', 'P', '1'};

bool validNameChar(uint8_t c, bool first) {
  if (c >= (uint8_t)'A' && c <= (uint8_t)'Z') return false;
  if (c >= (uint8_t)'a' && c <= (uint8_t)'z') return true;
  if (c == (uint8_t)'_') return true;
  if (first) return false;
  if (c >= (uint8_t)'0' && c <= (uint8_t)'9') return true;
  return c == (uint8_t)'-';
}

uint16_t entryOffset(uint16_t index) {
  return (uint16_t)(kPackHeaderSize + (uint32_t)index * kPackEntrySize);
}

const uint8_t* entryPtr(const PackView& view, uint16_t index) {
  return view.toc + (uint32_t)index * kPackEntrySize;
}

uint16_t codeStart(const PackView& view) {
  return (uint16_t)(kPackHeaderSize + view.header.tocSize);
}

}  // namespace

bool packNormalizeName(const char* name, uint8_t out[kPackNameLen]) {
  if (!name || !out) return false;

  uint8_t length = 0;
  while (name[length] != '\0') {
    if (length >= kPackNameLen) return false;
    uint8_t c = (uint8_t)name[length];
    if (c >= (uint8_t)'A' && c <= (uint8_t)'Z') {
      c = (uint8_t)(c - (uint8_t)'A' + (uint8_t)'a');
    }
    if (!validNameChar(c, length == 0)) return false;
    out[length] = c;
    length++;
  }
  if (length == 0) return false;

  memset(out + length, 0, (size_t)(kPackNameLen - length));
  return true;
}

bool packNameToText(const uint8_t name[kPackNameLen], char out[kPackNameLen + 1]) {
  if (!name || !out) return false;

  uint8_t length = 0;
  while (length < kPackNameLen && name[length] != 0) {
    if (!validNameChar(name[length], length == 0)) return false;
    out[length] = (char)name[length];
    length++;
  }
  if (length == 0) return false;
  for (uint8_t i = length; i < kPackNameLen; i++) {
    if (name[i] != 0) return false;
  }
  out[length] = '\0';
  return true;
}

bool parsePackToc(const uint8_t* data, uint16_t size, uint32_t fileSize,
                  PackView& out, PackError& error) {
  error = PackError::NONE;
  out.data = data;
  out.size = size;
  out.fileSize = fileSize;
  out.toc = nullptr;
  memset(&out.header, 0, sizeof(out.header));

  if (!data || size < kPackHeaderSize || fileSize < size) {
    error = PackError::SHORT;
    return false;
  }
  if (data[0] != kPackMagic[0] || data[1] != kPackMagic[1] ||
      data[2] != kPackMagic[2] || data[3] != kPackMagic[3]) {
    error = PackError::MAGIC;
    return false;
  }

  out.header.version = data[4];
  out.header.count = data[5];
  out.header.flags = data[6];
  out.header.reserved = data[7];
  out.header.tocSize = readU16(data + 8);
  out.header.tocCrc = readU32(data + 10);

  if (out.header.version != kPackVersion) {
    error = PackError::VERSION;
    return false;
  }
  if (out.header.count > kPackMaxEntries) {
    error = PackError::COUNT;
    return false;
  }
  if (out.header.flags != 0 || out.header.reserved != 0) {
    error = PackError::FLAGS;
    return false;
  }
  if (out.header.tocSize != (uint16_t)out.header.count * kPackEntrySize) {
    error = PackError::TOC_SIZE;
    return false;
  }
  if ((uint32_t)kPackHeaderSize + out.header.tocSize > size) {
    error = PackError::BOUNDS;
    return false;
  }

  out.toc = data + kPackHeaderSize;
  if (crc32(out.toc, out.header.tocSize) != out.header.tocCrc) {
    error = PackError::TOC_CRC;
    return false;
  }

  const uint16_t firstCode = codeStart(out);
  for (uint16_t i = 0; i < out.header.count; i++) {
    const uint8_t* entry = entryPtr(out, i);
    char text[kPackNameLen + 1];
    const uint8_t flags = entry[16];
    const uint16_t off = readU16(entry + 17);
    const uint16_t len = readU16(entry + 19);

    if (!packNameToText(entry, text)) {
      error = PackError::NAME;
      return false;
    }
    if ((flags & (uint8_t)~kPackFlagMask) != 0) {
      error = PackError::FLAGS;
      return false;
    }
    if (len == 0 || len > kMaxCodeBytes) {
      error = PackError::CODE_LEN;
      return false;
    }
    if (off < firstCode || (uint32_t)off + len > fileSize) {
      error = PackError::BOUNDS;
      return false;
    }

    for (uint16_t j = 0; j < i; j++) {
      const uint8_t* prev = entryPtr(out, j);
      if (memcmp(entry, prev, kPackNameLen) == 0) {
        error = PackError::DUPLICATE;
        return false;
      }
      const uint16_t prevOff = readU16(prev + 17);
      const uint16_t prevLen = readU16(prev + 19);
      if (off < prevOff + prevLen && prevOff < off + len) {
        error = PackError::OVERLAP;
        return false;
      }
    }
  }

  return true;
}

bool parsePack(const uint8_t* data, uint16_t size, PackView& out, PackError& error) {
  if (!parsePackToc(data, size, size, out, error)) return false;

  for (uint16_t i = 0; i < out.header.count; i++) {
    const uint8_t* entry = entryPtr(out, i);
    const uint16_t off = readU16(entry + 17);
    const uint16_t len = readU16(entry + 19);
    const uint32_t crc = readU32(entry + 21);
    if (crc32(data + off, len) != crc) {
      error = PackError::CODE_CRC;
      return false;
    }
  }
  return true;
}

bool verifyPackCode(const PackView& view, uint16_t index, const uint8_t* code,
                    uint16_t len) {
  if (!view.toc || index >= view.header.count || !code) return false;

  const uint8_t* entry = entryPtr(view, index);
  const uint16_t entryLen = readU16(entry + 19);
  const uint32_t crc = readU32(entry + 21);
  return len == entryLen && crc32(code, len) == crc;
}

bool findPackEntry(const PackView& view, const char* name, uint16_t& index) {
  uint8_t key[kPackNameLen];
  if (!view.toc || !packNormalizeName(name, key)) return false;

  for (uint16_t i = 0; i < view.header.count; i++) {
    if (memcmp(entryPtr(view, i), key, kPackNameLen) == 0) {
      index = i;
      return true;
    }
  }
  return false;
}

bool getPackEntry(const PackView& view, uint16_t index, PackEntry& out) {
  if (!view.toc || index >= view.header.count) return false;

  const uint8_t* entry = entryPtr(view, index);
  memcpy(out.name, entry, kPackNameLen);
  out.flags = entry[16];
  out.codeOff = readU16(entry + 17);
  out.codeLen = readU16(entry + 19);
  out.codeCrc = readU32(entry + 21);
  return true;
}

bool getPackCode(const PackView& view, uint16_t index, const uint8_t*& code, uint16_t& len) {
  if (!view.toc || index >= view.header.count) return false;

  const uint8_t* entry = entryPtr(view, index);
  const uint16_t off = readU16(entry + 17);
  const uint16_t size = readU16(entry + 19);
  if (off < codeStart(view) || (uint32_t)off + size > view.fileSize ||
      (uint32_t)off + size > view.size) {
    return false;
  }

  code = view.data + off;
  len = size;
  return true;
}

bool buildPack(const PackInputEntry* entries, uint8_t count, uint8_t* out,
               uint16_t outCapacity, uint16_t& outLen, PackError& error) {
  error = PackError::NONE;
  outLen = 0;

  if (!out) {
    error = PackError::CAPACITY;
    return false;
  }
  if (count > kPackMaxEntries) {
    error = PackError::COUNT;
    return false;
  }
  if (count > 0 && !entries) {
    error = PackError::COUNT;
    return false;
  }

  const uint16_t tocSize = (uint16_t)((uint16_t)count * kPackEntrySize);
  uint8_t names[kPackMaxEntries][kPackNameLen];
  uint32_t total = (uint32_t)kPackHeaderSize + tocSize;

  for (uint8_t i = 0; i < count; i++) {
    if (!packNormalizeName(entries[i].name, names[i])) {
      error = PackError::NAME;
      return false;
    }
    if ((entries[i].flags & (uint8_t)~kPackFlagMask) != 0) {
      error = PackError::FLAGS;
      return false;
    }
    if (entries[i].codeLen == 0 || entries[i].codeLen > kMaxCodeBytes ||
        !entries[i].code) {
      error = PackError::CODE_LEN;
      return false;
    }
    for (uint8_t j = 0; j < i; j++) {
      if (memcmp(names[i], names[j], kPackNameLen) == 0) {
        error = PackError::DUPLICATE;
        return false;
      }
    }
    total += entries[i].codeLen;
  }

  if (total > outCapacity || total > 0xffffu) {
    error = PackError::CAPACITY;
    return false;
  }

  memcpy(out, kPackMagic, sizeof(kPackMagic));
  out[4] = kPackVersion;
  out[5] = count;
  out[6] = 0;
  out[7] = 0;
  writeU16(out + 8, tocSize);
  writeU32(out + 10, 0);

  uint16_t codeOff = (uint16_t)(kPackHeaderSize + tocSize);
  for (uint8_t i = 0; i < count; i++) {
    uint8_t* entry = out + entryOffset(i);
    memcpy(entry, names[i], kPackNameLen);
    entry[16] = entries[i].flags;
    writeU16(entry + 17, codeOff);
    writeU16(entry + 19, entries[i].codeLen);
    writeU32(entry + 21, crc32(entries[i].code, entries[i].codeLen));
    memcpy(out + codeOff, entries[i].code, entries[i].codeLen);
    codeOff = (uint16_t)(codeOff + entries[i].codeLen);
  }

  writeU32(out + 10, crc32(out + kPackHeaderSize, tocSize));
  outLen = (uint16_t)total;
  return true;
}

const char* packErrorName(PackError error) {
  switch (error) {
    case PackError::NONE:
      return "none";
    case PackError::SHORT:
      return "short pack";
    case PackError::MAGIC:
      return "bad magic";
    case PackError::VERSION:
      return "unsupported version";
    case PackError::COUNT:
      return "invalid entry count";
    case PackError::TOC_SIZE:
      return "toc size mismatch";
    case PackError::TOC_CRC:
      return "toc crc mismatch";
    case PackError::NAME:
      return "invalid name";
    case PackError::DUPLICATE:
      return "duplicate name";
    case PackError::BOUNDS:
      return "code out of bounds";
    case PackError::CODE_CRC:
      return "code crc mismatch";
    case PackError::OVERLAP:
      return "code regions overlap";
    case PackError::CAPACITY:
      return "output buffer too small";
    case PackError::CODE_LEN:
      return "code length out of range";
    case PackError::FLAGS:
      return "unsupported flags";
    default:
      return "unknown";
  }
}

}  // namespace bytecode
}  // namespace vuesp
