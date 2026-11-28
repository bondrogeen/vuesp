// Host-side generator input: includes the single C++ script manifest and
// prints it as JSON. The Node wrapper turns this into TypeScript.
#include "../src/generated/scriptManifest.h"

#include <stdio.h>

using namespace vuesp::bytecode;

static void printString(const char* value) {
  putchar('"');
  if (value) {
    for (const char* p = value; *p; p++) {
      if (*p == '"' || *p == '\\') putchar('\\');
      putchar(*p);
    }
  }
  putchar('"');
}

int main() {
  printf("{\"events\":[");
  for (uint8_t i = 0; i < kScriptEventCount; i++) {
    const EventSpec& event = kScriptEvents[i];
    if (i) putchar(',');
    printf("{\"id\":%u,\"name\":", (unsigned)event.id);
    printString(event.name);
    printf(",\"params\":%u,\"args\":", (unsigned)event.params);
    printString(event.args);
    printf(",\"description\":");
    printString(event.description);
    putchar('}');
  }
  printf("],\"functions\":[");
  for (uint8_t i = 0; i < kScriptFunctionCount; i++) {
    const FunctionSpec& fn = kScriptFunctions[i];
    if (i) putchar(',');
    printf("{\"id\":%u,\"name\":", (unsigned)fn.id);
    printString(fn.name);
    printf(",\"minArgs\":%u,\"maxArgs\":%u,\"ret\":%u,\"description\":",
           (unsigned)fn.minArgs, (unsigned)fn.maxArgs, (unsigned)fn.ret);
    printString(fn.description);
    putchar('}');
  }
  printf("],\"limits\":{\"maxSavedScripts\":%u,\"cacheSlots\":%u,\"maxScripts\":%u,"
         "\"maxContexts\":%u,\"maxCodeBytes\":%u,\"stackSize\":%u,"
         "\"scratchBytes\":%u,\"maxStringLen\":%u,\"maxArrayLen\":%u,"
         "\"maxEventParams\":%u,\"instrPerTick\":%u,\"usPerTick\":%u,"
         "\"updateBudgetUs\":%u,\"vars\":{\"u\":%u,\"i\":%u,\"f\":%u,"
         "\"s\":%u,\"a\":%u,\"p\":%u,\"e\":%u,\"c\":%u}}}",
         (unsigned)kScriptLimits.maxSavedScripts,
         (unsigned)kScriptLimits.cacheSlots,
         (unsigned)kScriptLimits.maxScripts,
         (unsigned)kScriptLimits.maxContexts,
         (unsigned)kScriptLimits.maxCodeBytes,
         (unsigned)kScriptLimits.stackSize,
         (unsigned)kScriptLimits.scratchBytes,
         (unsigned)kScriptLimits.maxStringLen,
         (unsigned)kScriptLimits.maxArrayLen,
         (unsigned)kScriptLimits.maxEventParams,
         (unsigned)kScriptLimits.instrPerTick,
         (unsigned)kScriptLimits.usPerTick,
         (unsigned)kScriptLimits.updateBudgetUs,
         (unsigned)kScriptLimits.vars[0], (unsigned)kScriptLimits.vars[1],
         (unsigned)kScriptLimits.vars[2], (unsigned)kScriptLimits.vars[3],
         (unsigned)kScriptLimits.vars[4], (unsigned)kScriptLimits.vars[5],
         (unsigned)kScriptLimits.vars[6], (unsigned)kScriptLimits.vars[7]);
  return 0;
}
