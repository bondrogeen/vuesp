#include <stdio.h>
#include <string.h>

#include "../runner/bytecode/BytecodePack.h"

using namespace vuesp::bytecode;

static int failures = 0;

static void expect(bool condition, const char* message) {
  if (!condition) {
    printf("FAIL: %s\n", message);
    failures++;
  }
}

static void writeU16(uint8_t* data, uint16_t value) {
  data[0] = (uint8_t)(value & 0xff);
  data[1] = (uint8_t)((value >> 8) & 0xff);
}

static void writeU32(uint8_t* data, uint32_t value) {
  data[0] = (uint8_t)(value & 0xff);
  data[1] = (uint8_t)((value >> 8) & 0xff);
  data[2] = (uint8_t)((value >> 16) & 0xff);
  data[3] = (uint8_t)((value >> 24) & 0xff);
}

static void fixTocCrc(uint8_t* data) {
  const uint16_t tocSize = readU16(data + 8);
  writeU32(data + 10, crc32(data + kPackHeaderSize, tocSize));
}

static const uint8_t kCodeBoot[] = {0x56, 0x42, 0x43, 0x31, 1, 2, 3};
static const uint8_t kCodeBlink[] = {0x56, 0x42, 0x43, 0x31, 9, 8, 7, 6};

static uint16_t makeValid(uint8_t* out, uint16_t capacity) {
  const PackInputEntry entries[] = {
      {"boot", 0, kCodeBoot, sizeof(kCodeBoot)},
      {"Blink", kPackFlagEnabled, kCodeBlink, sizeof(kCodeBlink)},
  };
  uint16_t len = 0;
  PackError error = PackError::NONE;
  if (!buildPack(entries, 2, out, capacity, len, error)) return 0;
  return len;
}

static void testNameNormalize() {
  uint8_t key[kPackNameLen];
  char text[kPackNameLen + 1];

  expect(packNormalizeName("boot", key), "normalize boot");
  expect(packNameToText(key, text), "name to text boot");
  expect(strcmp(text, "boot") == 0, "boot text");

  expect(packNormalizeName("Boot", key), "normalize Boot");
  expect(packNameToText(key, text), "name to text Boot");
  expect(strcmp(text, "boot") == 0, "uppercase is lowercased");

  expect(packNormalizeName("UPPER_9-x", key), "normalize upper name");
  expect(packNameToText(key, text), "name to text upper");
  expect(strcmp(text, "upper_9-x") == 0, "normalized value");

  expect(packNormalizeName("a", key), "single letter name");
  expect(packNormalizeName("_x", key), "underscore name");
  expect(packNormalizeName("abcdefghijklmnop", key), "16 char name");
  expect(packNameToText(key, text), "16 char text");
  expect(strlen(text) == 16, "16 char length");

  expect(!packNormalizeName("", key), "empty name rejected");
  expect(!packNormalizeName("1abc", key), "digit first rejected");
  expect(!packNormalizeName("-abc", key), "dash first rejected");
  expect(!packNormalizeName("bad name", key), "space rejected");
  expect(!packNormalizeName("bad:name", key), "colon rejected");
  expect(!packNormalizeName("abcdefghijklmnopq", key), "17 char rejected");
  expect(!packNormalizeName(nullptr, key), "null name rejected");

  const uint8_t empty[kPackNameLen] = {0};
  expect(!packNameToText(empty, text), "empty key rejected");

  uint8_t upper[kPackNameLen] = {0};
  upper[0] = 'B';
  expect(!packNameToText(upper, text), "uppercase key rejected");

  uint8_t firstDigit[kPackNameLen] = {0};
  firstDigit[0] = '1';
  expect(!packNameToText(firstDigit, text), "digit key rejected");

  uint8_t padded[kPackNameLen] = {0};
  padded[0] = 'b';
  padded[1] = 'o';
  padded[2] = 'o';
  padded[3] = 't';
  padded[5] = 'x';
  expect(!packNameToText(padded, text), "non-zero padding rejected");
}

static void testBuildParseLookup() {
  uint8_t pack[20000];
  PackError error = PackError::NONE;
  uint16_t len = 0;

  const PackInputEntry entries[] = {
      {"boot", 0, kCodeBoot, sizeof(kCodeBoot)},
      {"Blink", kPackFlagEnabled, kCodeBlink, sizeof(kCodeBlink)},
  };
  expect(buildPack(entries, 2, pack, sizeof(pack), len, error), "build valid pack");
  expect(error == PackError::NONE, "build error none");
  expect(len == kPackHeaderSize + 2 * kPackEntrySize + sizeof(kCodeBoot) +
                   sizeof(kCodeBlink),
         "pack length");

  PackView view;
  expect(parsePack(pack, len, view, error), "parse valid pack");
  expect(error == PackError::NONE, "parse error none");
  expect(view.header.version == kPackVersion, "version");
  expect(view.header.count == 2, "count");
  expect(view.header.tocSize == 2 * kPackEntrySize, "toc size");
  expect(view.header.tocCrc == crc32(view.toc, view.header.tocSize), "toc crc");

  uint16_t index = 0;
  expect(findPackEntry(view, "boot", index), "find boot");
  expect(index == 0, "boot index");
  expect(findPackEntry(view, "BOOT", index), "find BOOT");
  expect(index == 0, "BOOT resolves to boot");
  expect(findPackEntry(view, "blink", index), "find blink");
  expect(index == 1, "blink index");
  expect(!findPackEntry(view, "missing", index), "missing name");

  PackEntry entry;
  expect(getPackEntry(view, 1, entry), "get entry 1");
  char text[kPackNameLen + 1];
  expect(packNameToText(entry.name, text), "entry 1 name");
  expect(strcmp(text, "blink") == 0, "entry 1 normalized name");
  expect(entry.flags == kPackFlagEnabled, "entry 1 flags");
  expect(entry.codeLen == sizeof(kCodeBlink), "entry 1 code length");
  expect(entry.codeCrc == crc32(kCodeBlink, sizeof(kCodeBlink)), "entry 1 code crc");
  expect(entry.codeOff == kPackHeaderSize + 2 * kPackEntrySize + sizeof(kCodeBoot),
         "entry 1 offset");

  expect(!getPackEntry(view, 2, entry), "entry index bounds");
  const uint8_t* code = nullptr;
  uint16_t codeLen = 0;
  expect(getPackCode(view, 0, code, codeLen), "get code 0");
  expect(codeLen == sizeof(kCodeBoot), "code 0 length");
  expect(memcmp(code, kCodeBoot, sizeof(kCodeBoot)) == 0, "code 0 bytes");
  expect(!getPackCode(view, 2, code, codeLen), "code index bounds");
}

static void testEmptyPack() {
  uint8_t pack[32];
  PackError error = PackError::NONE;
  uint16_t len = 0;

  expect(buildPack(nullptr, 0, pack, sizeof(pack), len, error), "build empty pack");
  expect(len == kPackHeaderSize, "empty pack length");
  expect(pack[5] == 0, "empty count");

  PackView view;
  expect(parsePack(pack, len, view, error), "parse empty pack");
  expect(view.header.count == 0, "empty count parsed");
  expect(view.header.tocSize == 0, "empty toc size");
  expect(view.header.tocCrc == 0, "empty toc crc");

  uint16_t index = 0;
  expect(!findPackEntry(view, "boot", index), "empty pack has no boot");
}

static void testMaxEntries() {
  uint8_t pack[20000];
  char names[kPackMaxEntries][17];
  PackInputEntry entries[kPackMaxEntries];
  const uint8_t code[] = {0x42};

  for (uint8_t i = 0; i < kPackMaxEntries; i++) {
    snprintf(names[i], sizeof(names[i]), "s%u", (unsigned)i);
    entries[i].name = names[i];
    entries[i].flags = 0;
    entries[i].code = code;
    entries[i].codeLen = sizeof(code);
  }

  PackError error = PackError::NONE;
  uint16_t len = 0;
  expect(buildPack(entries, kPackMaxEntries, pack, sizeof(pack), len, error),
         "build max entries");

  PackView view;
  expect(parsePack(pack, len, view, error), "parse max entries");
  expect(view.header.count == kPackMaxEntries, "max entries count");

  uint16_t index = 0;
  expect(findPackEntry(view, "s7", index), "find s7");
  expect(index == 7, "s7 index");

  expect(!buildPack(entries, kPackMaxEntries + 1, pack, sizeof(pack), len, error),
         "reject 16 entries");
  expect(error == PackError::COUNT, "16 entries error");
}

static void testDuplicate() {
  uint8_t pack[20000];
  PackError error = PackError::NONE;
  uint16_t len = 0;

  const PackInputEntry entries[] = {
      {"boot", 0, kCodeBoot, sizeof(kCodeBoot)},
      {"Boot", 0, kCodeBlink, sizeof(kCodeBlink)},
  };
  expect(!buildPack(entries, 2, pack, sizeof(pack), len, error), "build duplicate");
  expect(error == PackError::DUPLICATE, "build duplicate error");

  len = makeValid(pack, sizeof(pack));
  expect(len > 0, "duplicate fixture built");
  uint8_t* second = pack + kPackHeaderSize + kPackEntrySize;
  memset(second, 0, kPackNameLen);
  memcpy(second, "boot", 4);
  fixTocCrc(pack);

  PackView view;
  expect(!parsePack(pack, len, view, error), "parse duplicate");
  expect(error == PackError::DUPLICATE, "parse duplicate error");
}

static void testNameErrors() {
  uint8_t pack[20000];
  PackError error = PackError::NONE;
  uint16_t len = 0;

  const PackInputEntry bad[] = {
      {"abcdefghijklmnopq", 0, kCodeBoot, sizeof(kCodeBoot)},
  };
  expect(!buildPack(bad, 1, pack, sizeof(pack), len, error), "build overlong name");
  expect(error == PackError::NAME, "overlong name error");

  const PackInputEntry badChar[] = {
      {"bad:name", 0, kCodeBoot, sizeof(kCodeBoot)},
  };
  expect(!buildPack(badChar, 1, pack, sizeof(pack), len, error), "build bad char");
  expect(error == PackError::NAME, "bad char error");

  len = makeValid(pack, sizeof(pack));
  expect(len > 0, "name fixture built");

  uint8_t* first = pack + kPackHeaderSize;
  first[0] = 'B';
  fixTocCrc(pack);

  PackView view;
  expect(!parsePack(pack, len, view, error), "parse uppercase name");
  expect(error == PackError::NAME, "uppercase name error");

  len = makeValid(pack, sizeof(pack));
  first = pack + kPackHeaderSize;
  first[1] = ':';
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "parse invalid name");
  expect(error == PackError::NAME, "invalid name error");
}

static void testCorruption() {
  uint8_t pack[20000];
  PackError error = PackError::NONE;
  PackView view;

  expect(!parsePack(nullptr, 0, view, error), "null pack");
  expect(error == PackError::SHORT, "null pack error");

  uint16_t len = makeValid(pack, sizeof(pack));
  expect(len > 0, "corruption fixture built");
  expect(!parsePack(pack, 5, view, error), "short pack");
  expect(error == PackError::SHORT, "short pack error");

  len = makeValid(pack, sizeof(pack));
  pack[0] = 'X';
  expect(!parsePack(pack, len, view, error), "bad magic");
  expect(error == PackError::MAGIC, "bad magic error");

  len = makeValid(pack, sizeof(pack));
  pack[4] = 2;
  expect(!parsePack(pack, len, view, error), "bad version");
  expect(error == PackError::VERSION, "bad version error");

  len = makeValid(pack, sizeof(pack));
  pack[6] = 1;
  expect(!parsePack(pack, len, view, error), "header flags");
  expect(error == PackError::FLAGS, "header flags error");

  len = makeValid(pack, sizeof(pack));
  pack[7] = 1;
  expect(!parsePack(pack, len, view, error), "reserved flags");
  expect(error == PackError::FLAGS, "reserved flags error");

  len = makeValid(pack, sizeof(pack));
  pack[5] = kPackMaxEntries + 1;
  expect(!parsePack(pack, len, view, error), "bad count");
  expect(error == PackError::COUNT, "bad count error");

  len = makeValid(pack, sizeof(pack));
  writeU16(pack + 8, 2 * kPackEntrySize - 1);
  expect(!parsePack(pack, len, view, error), "bad toc size");
  expect(error == PackError::TOC_SIZE, "bad toc size error");

  len = makeValid(pack, sizeof(pack));
  pack[10] ^= 0xff;
  expect(!parsePack(pack, len, view, error), "bad toc crc");
  expect(error == PackError::TOC_CRC, "bad toc crc error");

  len = makeValid(pack, sizeof(pack));
  const uint16_t firstCode = (uint16_t)(kPackHeaderSize + 2 * kPackEntrySize);
  pack[firstCode] ^= 0xff;
  expect(!parsePack(pack, len, view, error), "bad code crc");
  expect(error == PackError::CODE_CRC, "bad code crc error");

  len = makeValid(pack, sizeof(pack));
  writeU16(pack + kPackHeaderSize + 19, 0);
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "zero code length");
  expect(error == PackError::CODE_LEN, "zero code length error");

  len = makeValid(pack, sizeof(pack));
  writeU16(pack + kPackHeaderSize + 19, kMaxCodeBytes + 1);
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "code length over max");
  expect(error == PackError::CODE_LEN, "code length over max error");

  len = makeValid(pack, sizeof(pack));
  pack[kPackHeaderSize + 16] = 0x80;
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "entry flags");
  expect(error == PackError::FLAGS, "entry flags error");
}

static void testBoundsOverlap() {
  uint8_t pack[20000];
  PackError error = PackError::NONE;
  PackView view;

  uint16_t len = makeValid(pack, sizeof(pack));
  expect(len > 0, "bounds fixture built");

  writeU16(pack + kPackHeaderSize + 17, 0xffff);
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "code offset out of bounds");
  expect(error == PackError::BOUNDS, "code offset error");

  len = makeValid(pack, sizeof(pack));
  writeU16(pack + kPackHeaderSize + 17, 0);
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "code offset into toc");
  expect(error == PackError::BOUNDS, "code offset into toc error");

  len = makeValid(pack, sizeof(pack));
  const uint16_t firstOff = readU16(pack + kPackHeaderSize + 17);
  writeU16(pack + kPackHeaderSize + kPackEntrySize + 17, firstOff);
  fixTocCrc(pack);
  expect(!parsePack(pack, len, view, error), "overlapping code");
  expect(error == PackError::OVERLAP, "overlapping code error");
}

static void testCapacity() {
  uint8_t pack[20000];
  PackError error = PackError::NONE;
  uint16_t len = 0;

  const PackInputEntry entries[] = {
      {"boot", 0, kCodeBoot, sizeof(kCodeBoot)},
      {"blink", 0, kCodeBlink, sizeof(kCodeBlink)},
  };
  const uint16_t needed =
      kPackHeaderSize + 2 * kPackEntrySize + sizeof(kCodeBoot) + sizeof(kCodeBlink);

  expect(!buildPack(entries, 2, pack, needed - 1, len, error), "capacity too small");
  expect(error == PackError::CAPACITY, "capacity error");
  expect(!buildPack(entries, 2, nullptr, needed, len, error), "null output");
  expect(error == PackError::CAPACITY, "null output error");

  expect(buildPack(entries, 2, pack, needed, len, error), "exact capacity");
  expect(len == needed, "exact capacity length");

  expect(!buildPack(nullptr, 1, pack, sizeof(pack), len, error), "null entries");
  expect(error == PackError::COUNT, "null entries error");

  const PackInputEntry nullCode[] = {
      {"boot", 0, nullptr, sizeof(kCodeBoot)},
  };
  expect(!buildPack(nullCode, 1, pack, sizeof(pack), len, error), "null code");
  expect(error == PackError::CODE_LEN, "null code error");

  const PackInputEntry zeroLen[] = {
      {"boot", 0, kCodeBoot, 0},
  };
  expect(!buildPack(zeroLen, 1, pack, sizeof(pack), len, error), "zero length");
  expect(error == PackError::CODE_LEN, "zero length error");

  const PackInputEntry badFlags[] = {
      {"boot", 0x80, kCodeBoot, sizeof(kCodeBoot)},
  };
  expect(!buildPack(badFlags, 1, pack, sizeof(pack), len, error), "bad flags");
  expect(error == PackError::FLAGS, "bad flags error");
}

int main() {
  testNameNormalize();
  testBuildParseLookup();
  testEmptyPack();
  testMaxEntries();
  testDuplicate();
  testNameErrors();
  testCorruption();
  testBoundsOverlap();
  testCapacity();

  if (failures == 0) {
    printf("All bytecode pack tests passed\n");
    return 0;
  }
  printf("%d bytecode pack test(s) failed\n", failures);
  return 1;
}
