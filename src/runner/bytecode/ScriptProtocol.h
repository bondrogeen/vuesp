#ifndef VUESP_SCRIPT_PROTOCOL_H
#define VUESP_SCRIPT_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

// Serialized bytecode control plane: WebSocket commands are queued here and
// executed from loop(), so the VM and the script pack stay single-owner.
class ScriptProtocol {
 public:
  void begin();
  void loop();

  bool onWsCommand(const uint8_t* data, size_t len, uint32_t clientId);
  bool queueSave(const char* name, uint16_t requestId, uint8_t flags);

  // True while a SAVE request is queued or being executed. The HTTP upload
  // path uses it to avoid overwriting the temporary file under a running save.
  bool isBusy() const;

 private:
  static constexpr uint8_t kQueueSize = 4;

  struct Request {
    uint8_t action;
    uint8_t flags;
    uint16_t requestId;
    uint32_t clientId;
    char name[16];
  };

  Request _queue[kQueueSize];
  uint8_t _head = 0;
  uint8_t _tail = 0;
  uint8_t _count = 0;
  bool _busy = false;

  bool push(const Request& request);
  bool pop(Request& request);
  void process(const Request& request);
  void sendStatus(const Request& request, uint8_t result);
  void sendBusy(uint32_t clientId, uint16_t requestId);
};

extern ScriptProtocol scriptProtocol;

#endif
