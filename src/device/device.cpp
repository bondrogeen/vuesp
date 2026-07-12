#include "./device.h"

// extern ScriptRunner scriptRunner;

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE, 0, 100, 1760640900, "1"};
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

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
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
