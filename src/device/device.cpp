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
  if (port->value) {
    // uint32_t event = scriptRunner.hash("EVT1");
    // scriptRunner.emitEvent(event);
    // uint32_t event2 = scriptRunner.hash("EVT2");
    // scriptRunner.emitEvent(event2);
    // scriptRunner.printSlotInfo();

    scriptRunner.registerScript(2, "$a0={10,20,30};$v0=len($a0);$display=$v0");
    scriptRunner.runScript(2);
    // scriptRunner.runScript(1);
  }
}

void getADC() {
  device.analog = analogRead(A0);
}

void getData() {
  getADC();
}
uint32_t counter = 0;
bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  Serial.print("[DATA] ID:");
  Serial.print(id);
  Serial.print(" Kind:");
  Serial.print(kind);
  Serial.print(" Write:");
  Serial.println(write ? "YES" : "NO");

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        char buf[65];
        strncpy(buf, (char*)value.stringVal.data, value.stringVal.len);
        buf[value.stringVal.len] = '\0';
        Serial.print("[DISPLAY] ");
        Serial.println(buf);
      } else if (kind == KIND_INT || kind == KIND_UINT) {
        Serial.print("[DISPLAY] ");
        Serial.println(value.intVal);
      } else if (kind == KIND_FLOAT) {
        Serial.print("[DISPLAY] ");
        Serial.println(value.floatVal);
      }
    }
    return true;
  }

  if (strcmp(id, "$counter") == 0) {
    if (write) {
      if (kind == KIND_INT || kind == KIND_UINT) {
        counter = (uint32_t)value.intVal;
        Serial.print("[COUNTER] SET TO ");
        Serial.println(counter);
      }
    } else {
      value.uintVal = counter;
      Serial.print("[COUNTER] READ = ");
      Serial.println(counter);
    }
    return true;
  }

  return false;
}

void setupDevice() {
  scriptRunner.setDataProvider(dataProvider);
  // scriptRunner.registerScript(1, "$p13=255,wait(100u),$p13=0,on('button'),$serial='Pressed',$p13=255,wait(1s),$p13=0,end,on('button3'),$serial='Pressed',$p14=1,wait(2s),$p14=0,end");
  scriptRunner.runScript(1);

    //   scriptRunner.registerScript(2, "on('EVT1'),$display='H1',end,on('EVT2'),$display='H2',end");
    // scriptRunner.runScript(2);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();

    // buffer.push(random(-55, 55));
    // Serial.println("Test 1: [3]14:1,p10,14:0,p10]");
    // scriptRunner.addScript(3, "[3]14:1,p10,14:0,p10]", RESTART);
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  };

  if (tasks[KEY_DEVICE]) {
    if (device.command == DEVICE_COMMAND_SAVE) writeFile(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    if (device.command == DEVICE_COMMAND_TEXT) sendNotification();

    device.command = 0;
    tasks[KEY_DEVICE] = 0;
    onSendDevice();
  };
}
