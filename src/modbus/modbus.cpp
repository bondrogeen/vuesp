#include "./modbus.h"

void uint16ToByte(uint16_t crc, uint8_t* bytes) {
  bytes[1] = (crc & 0xFF00) >> 8;
  bytes[0] = crc & 0x00FF;
}

uint16_t getCrc16(uint8_t* data, uint16_t len) {
  uint8_t lo;
  union {
    uint16_t value;
    struct {
      uint8_t lo, hi;
    } bytes;
  } crc;

  crc.value = 0xFFFF;

  while (len--) {
    lo = crc.bytes.lo;
    crc.bytes.lo = crc.bytes.hi;
    crc.bytes.hi = lo ^ *data++;
    uint8_t mask = 1;
    if (crc.bytes.hi & mask) crc.value ^= 0x0240;
    if (crc.bytes.hi & (mask << 1)) crc.value ^= 0x0480;
    if (crc.bytes.hi & (mask << 2)) crc.bytes.hi ^= 0x09;
    if (crc.bytes.hi & (mask << 3)) crc.bytes.hi ^= 0x12;
    if (crc.bytes.hi & (mask << 4)) crc.bytes.hi ^= 0x24;
    if (crc.bytes.hi & (mask << 5)) crc.bytes.hi ^= 0x48;
    if (crc.bytes.hi & (mask << 6)) crc.bytes.hi ^= 0x90;
    if (crc.bytes.hi & (mask << 7)) crc.value ^= 0x2001;
  }
  return crc.value;
}
