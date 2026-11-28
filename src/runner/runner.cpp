#include "./runner.h"

#include "../const/define.h"
#include "../tasks/tasks.h"
#include "./bytecode/BytecodeService.h"
#include "./bytecode/ScriptProtocol.h"

void onScriptComplete(uint8_t slot, uint8_t id) {
#if VUESP_LOG_LEVEL >= 2
  Serial.print("Script ");
  Serial.print(id);
  Serial.print(" completed in slot ");
  Serial.println(slot);
#else
  (void)slot;
#endif

  scriptProtocol.sendState(id);
}

void setupRunner() {
  bytecodeService.vm().setLogProvider(sendLog);
  bytecodeService.vm().setCompleteCallback(onScriptComplete);
}
