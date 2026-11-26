#ifndef VUESP_BYTECODE_SERVICE_H
#define VUESP_BYTECODE_SERVICE_H

#include "../../const/bytecode.h"

#if ENABLE_BYTECODE_VM

#include <stdint.h>

#include "./BytecodePack.h"
#include "./BytecodeVM.h"

static_assert(VBC_CACHE_SLOTS >= 1 && VBC_CACHE_SLOTS <= 15,
              "VBC_CACHE_SLOTS must be between 1 and 15");

class BytecodeService {
 public:
  void begin();
  void loop(uint32_t now);
  vuesp::bytecode::BytecodeVM& vm() { return _vm; }

  bool saveScript(const char* name, const uint8_t* data, uint16_t len);
  bool removeScript(const char* name);
  bool runScript(const char* name);
  bool stopScript(const char* name);
  void stopAll();
  bool emitEvent(const char* name, uint8_t count, const int32_t* params);
  bool readScript(const char* name, uint8_t* buffer, uint16_t maxLen, uint16_t& len);
  uint8_t scriptCount() const;
  uint8_t savedCount() const;

  String capabilitiesJson() const;
  String listJson() const;
  String statusJson() const;

 private:
  static constexpr uint8_t kCacheSlots = VBC_CACHE_SLOTS;
  static constexpr uint16_t kTocBufferSize =
      vuesp::bytecode::kPackHeaderSize +
      (uint16_t)vuesp::bytecode::kPackMaxEntries *
          vuesp::bytecode::kPackEntrySize;

  struct Slot {
    bool used;
    uint8_t id;
    uint16_t len;
    uint8_t data[vuesp::bytecode::kMaxCodeBytes];
  };

  struct ScriptRef {
    bool used;
    uint8_t id;
    uint8_t name[vuesp::bytecode::kPackNameLen];
  };

  vuesp::bytecode::BytecodeVM _vm;
  Slot _slots[kCacheSlots];
  ScriptRef _refs[vuesp::bytecode::kPackMaxEntries];

  uint8_t _toc[kTocBufferSize];
  uint16_t _tocLen;
  vuesp::bytecode::PackView _packView;
  uint8_t _packCount;
  bool _packValid;
  vuesp::bytecode::PackError _packError;

  bool loadToc();
  bool savePack(const uint8_t name[vuesp::bytecode::kPackNameLen],
                const uint8_t* data, uint16_t len);
  bool removePackEntry(uint16_t index, uint8_t removedId);
  bool commitPack(const String& tmp);
  bool readBlob(uint16_t index, uint8_t* buffer, uint16_t maxLen, uint16_t& len,
                vuesp::bytecode::VmError& error);
  bool resolveScript(const char* name, uint8_t& id, const uint8_t*& data,
                     uint16_t& size, vuesp::bytecode::VmError& error);
  static bool resolveThunk(void* userData, const char* name, uint8_t& id,
                           const uint8_t*& data, uint16_t& size,
                           vuesp::bytecode::VmError& error);

  void rebuildRefs();
  int8_t findSlot(uint8_t id) const;
  int8_t findFreeSlot() const;
  int8_t loadEntryToSlot(uint16_t index, uint8_t id,
                         vuesp::bytecode::VmError& error);
  int8_t findRef(const char* name) const;
  int8_t findRefByKey(const uint8_t name[vuesp::bytecode::kPackNameLen]) const;
  int8_t findRefById(uint8_t id) const;
};

extern BytecodeService bytecodeService;

#endif  // ENABLE_BYTECODE_VM

#endif
