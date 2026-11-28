#ifndef VUESP_SCRIPT_MANIFEST_H
#define VUESP_SCRIPT_MANIFEST_H

#include <stdint.h>

#include "../const/features.h"
#include "../runner/bytecode/BytecodeCapabilities.h"

// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Run: node scripts/config/generate.mjs

struct ScriptLimits {
  uint8_t maxSavedScripts;
  uint8_t cacheSlots;
  uint8_t maxScripts;
  uint8_t maxContexts;
  uint16_t maxCodeBytes;
  uint16_t stackSize;
  uint16_t scratchBytes;
  uint8_t maxStringLen;
  uint8_t maxArrayLen;
  uint8_t vars[8];  // u, i, f, s, a, p, e, c
  uint8_t maxEventParams;
  uint16_t instrPerTick;
  uint16_t usPerTick;
  uint16_t updateBudgetUs;
};

static const ScriptLimits kScriptLimits = {
    VBC_MAX_SAVED_SCRIPTS,
    VBC_CACHE_SLOTS,
    VBC_MAX_SCRIPTS,
    VBC_MAX_CONTEXTS,
    VBC_MAX_CODE_BYTES,
    VBC_STACK_SIZE,
    VBC_SCRATCH_BYTES,
    MAX_STRING_LEN,
    MAX_ARRAY_SIZE,
    {MAX_UINT_VARS, MAX_INT_VARS, MAX_FLOAT_VARS, MAX_STRING_VARS, MAX_ARRAY_VARS,
     40, 4, 4},
    MAX_EVENT_PARAMS,
    32,
    300,
    3000,
};

static const vuesp::bytecode::EventSpec kScriptEvents[] = {
    {1, "btn", 2, "gpio,value", "button edge: press or release"},
    {2, "btn_c", 3, "gpio,value,count", "button click with click count"},
    {3, "btn_l", 2, "gpio,value", "button long press"},
    {4, "btn_r", 2, "gpio,value", "button hold repeat"},
#if VUESP_MODBUS
    {5, "modbus", 0, "", "Modbus event"},
#endif
    {6, "time", 2, "hours,minutes", "device uptime clock"},
};

static const vuesp::bytecode::FunctionSpec kScriptFunctions[] = {
    {1, "fade", 3, 3, vuesp::bytecode::VALUE_INT, "PWM fade"},
    {2, "udp", 1, 2, vuesp::bytecode::VALUE_INT, "UDP send"},
    {3, "http", 2, 5, vuesp::bytecode::VALUE_INT, "HTTP request"},
#if VUESP_MODBUS
    {4, "var", 1, 2, vuesp::bytecode::VALUE_FLOAT, "read a Modbus variable"},
#endif
#if VUESP_MODBUS
    {5, "modbus", 2, 3, vuesp::bytecode::VALUE_FLOAT, "Modbus operation"},
#endif
};

static const uint8_t kScriptEventCount =
    (uint8_t)(sizeof(kScriptEvents) / sizeof(kScriptEvents[0]));
static const uint8_t kScriptFunctionCount =
    (uint8_t)(sizeof(kScriptFunctions) / sizeof(kScriptFunctions[0]));

#endif
