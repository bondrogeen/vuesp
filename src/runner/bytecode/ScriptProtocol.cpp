#include "./ScriptProtocol.h"

#include <LittleFS.h>
#include <string.h>

#include "../../const/protocol.h"
#include "../../webserver/webserver.h"
#include "./BytecodeService.h"

static const char kUploadPath[] = "/tmp/upload.vbc";

void ScriptProtocol::begin() {
  _head = 0;
  _tail = 0;
  _count = 0;
  _busy = false;
}

bool ScriptProtocol::push(const Request& request) {
  if (_count >= kQueueSize) return false;
  _queue[_tail] = request;
  _tail = (uint8_t)((_tail + 1) % kQueueSize);
  _count++;
  return true;
}

bool ScriptProtocol::pop(Request& request) {
  if (_count == 0) return false;
  request = _queue[_head];
  _head = (uint8_t)((_head + 1) % kQueueSize);
  _count--;
  return true;
}

void ScriptProtocol::loop() {
  Request request;
  if (!pop(request)) return;
  _busy = true;
  process(request);
  _busy = false;
}

bool ScriptProtocol::isBusy() const {
  if (_busy) return true;
  uint8_t index = _head;
  for (uint8_t i = 0; i < _count; i++) {
    if (_queue[index].action == SCRIPTACT_SAVE) return true;
    index = (uint8_t)((index + 1) % kQueueSize);
  }
  return false;
}

bool ScriptProtocol::onWsCommand(const uint8_t* data, size_t len,
                                 uint32_t clientId) {
  if (!data || len != sizeof(ScriptCmd)) return false;
  const ScriptCmd& command = *(const ScriptCmd*)data;
  if (command.key != KEY_SCRIPTCMD || command.action >= SCRIPTACT_END) {
    return false;
  }

  Request request = {};
  request.action = command.action;
  request.flags = command.flags;
  request.requestId = command.requestId;
  request.clientId = clientId;
  strncpy(request.name, command.name, sizeof(request.name) - 1);

  if (!push(request)) {
    sendBusy(clientId, command.requestId);
    return false;
  }
  return true;
}

bool ScriptProtocol::queueSave(const char* name, uint16_t requestId,
                               uint8_t flags) {
  if (!name) return false;
  Request request = {};
  request.action = SCRIPTACT_SAVE;
  request.flags = flags;
  request.requestId = requestId;
  request.clientId = 0;
  strncpy(request.name, name, sizeof(request.name) - 1);
  return push(request);
}

void ScriptProtocol::sendStatus(const Request& request, uint8_t result) {
  ScriptStatus status = {};
  status.key = KEY_SCRIPTSTATUS;
  status.result = result;
  status.requestId = request.requestId;
  status.state = bytecodeService.scriptState(request.name);
  status.packError = bytecodeService.packErrorCode();
  status.error = bytecodeService.lastErrorCode();
  status.errorPc = bytecodeService.lastErrorPc();
  strncpy(status.name, request.name, sizeof(status.name) - 1);
  if (request.clientId) {
    wsSendTo(request.clientId, (uint8_t*)&status, sizeof(status));
  } else {
    wsSendAll((uint8_t*)&status, sizeof(status));
  }
}

void ScriptProtocol::sendState(uint8_t id) {
  char name[vuesp::bytecode::kPackNameLen + 1] = {0};
  if (!bytecodeService.scriptName(id, name)) return;

  Request request = {};
  request.action = SCRIPTACT_STATUS;
  request.requestId = 0;
  strncpy(request.name, name, sizeof(request.name) - 1);
  sendStatus(request, SCRIPTRES_OK);
}

void ScriptProtocol::sendBusy(uint32_t clientId, uint16_t requestId) {
  ScriptStatus status = {};
  status.key = KEY_SCRIPTSTATUS;
  status.result = SCRIPTRES_BUSY;
  status.requestId = requestId;
  status.packError = bytecodeService.packErrorCode();
  status.error = bytecodeService.lastErrorCode();
  status.errorPc = bytecodeService.lastErrorPc();
  if (clientId) {
    wsSendTo(clientId, (uint8_t*)&status, sizeof(status));
  } else {
    wsSendAll((uint8_t*)&status, sizeof(status));
  }
}

void ScriptProtocol::process(const Request& request) {
  uint8_t result = SCRIPTRES_OK;

  switch (request.action) {
    case SCRIPTACT_STATUS:
      break;
    case SCRIPTACT_RUN:
      if (!bytecodeService.runScript(request.name)) result = SCRIPTRES_INVALID;
      break;
    case SCRIPTACT_STOP:
      if (!bytecodeService.stopScript(request.name)) result = SCRIPTRES_NOT_FOUND;
      break;
    case SCRIPTACT_STOP_ALL:
      bytecodeService.stopAll();
      break;
    case SCRIPTACT_REMOVE:
      if (!bytecodeService.removeScript(request.name)) result = SCRIPTRES_NOT_FOUND;
      break;
    case SCRIPTACT_SAVE:
      result = bytecodeService.saveScriptFromFile(
          request.name, kUploadPath, (request.flags & SCRIPT_FLAG_RUN) != 0);
      LittleFS.remove(kUploadPath);
      break;
    default:
      result = SCRIPTRES_INVALID;
      break;
  }

  sendStatus(request, result);
}

ScriptProtocol scriptProtocol;
