#ifndef VUESP_BYTECODE_SERVICE_H
#define VUESP_BYTECODE_SERVICE_H

#include "../../const/bytecode.h"

#if ENABLE_BYTECODE_VM

#include <stdint.h>

#include "./BytecodeVM.h"

static_assert(VBC_CACHE_SLOTS >= 1 && VBC_CACHE_SLOTS <= 15,
              "VBC_CACHE_SLOTS must be between 1 and 15");

class BytecodeService {
 public:
  void begin();
  void loop(uint32_t now);
  vuesp::bytecode::BytecodeVM& vm() { return _vm; }

  bool saveScript(uint8_t id, const uint8_t* data, uint16_t len);
  bool removeScript(uint8_t id);
  bool runScript(uint8_t id);
  bool stopScript(uint8_t id);
  void stopAll();
  bool emitEvent(const char* name, uint8_t count, const int32_t* params);
  bool readScript(uint8_t id, uint8_t* buffer, uint16_t maxLen, uint16_t& len);
  uint8_t scriptCount() const;

  String capabilitiesJson() const;
  String listJson() const;
  String statusJson() const;

 private:
  static constexpr uint8_t kCacheSlots = VBC_CACHE_SLOTS;

  struct Slot {
    bool used;
    uint8_t id;
    uint16_t len;
    uint8_t data[vuesp::bytecode::kMaxCodeBytes];
  };

  vuesp::bytecode::BytecodeVM _vm;
  Slot _slots[kCacheSlots];

  String pathFor(uint8_t id) const;
  int8_t findSlot(uint8_t id) const;
  int8_t findFreeSlot() const;
  int8_t loadFileToSlot(uint8_t id);
  bool loadFile(uint8_t id, uint8_t* buffer, uint16_t maxLen, uint16_t& len);
};

extern BytecodeService bytecodeService;

#endif  // ENABLE_BYTECODE_VM

#endif
