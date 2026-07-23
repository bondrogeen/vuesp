#include "./device.h"

// extern ScriptRunner scriptRunner;

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE, 0, 100, 1760640900, 2, "text"};
uint32_t lastTimeDevice = 0;

// CircularBuffer<int8_t, 256> buffer(myBuffer);

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

// only port.interrupt == GPIO_INTERRUPT_CHANGE
void deviceGPIO(Port* port) {
  // Serial.print(port->gpio);
  // Serial.println(port->value);
  // if (port->value) {
  //   Serial.print(port->gpio);
  // $v0=0;while:$v0<10;$p14=$v0;wait(1);$v0=$v0+1;$display=$v0;end
  // }
}

uint32_t counter = 0;
bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        char buf[65];
        strncpy(buf, (char*)value.stringVal.data, value.stringVal.len);
        buf[value.stringVal.len] = '\0';
      } else if (kind == KIND_INT || kind == KIND_UINT) {
      } else if (kind == KIND_FLOAT) {
      }
    }
    return true;
  }

  if (strcmp(id, "$counter") == 0) {
    if (write) {
      if (kind == KIND_INT || kind == KIND_UINT) {
        counter = (uint32_t)value.intVal;
      }
    } else {
      value.uintVal = counter;
    }
    return true;
  }
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (paramCount < 2) return false;

  const char* method = params[0].stringVal.data;
  const char* url = params[1].stringVal.data;
  const int32_t count = params[2].intVal;
  const uint32_t num = params[3].uintVal;

  const uint8_t* data = params[4].arrayVal.data;
  uint8_t len = params[4].arrayVal.len;

  for (uint8_t i = 0; i < len; i++) {
    Serial.print(data[i]);
  }

  Serial.println("method");
  Serial.println(method);
  Serial.println(url);
  Serial.println(count);
  Serial.println(num);

  result.type = VAL_INT;
  result.intVal = 200;
  return true;
}

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    onSendDevice();
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  };

  if (tasks[KEY_DEVICE]) {
    if (device.command == DEVICE_COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    // if (device.command == DEVICE_COMMAND_TEXT) sendNotification();

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}
