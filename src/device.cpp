#include "./include/device.h"

extern ScriptRunner scriptRunner;

Buffer myBuffer = {KEY_BUFFER};

Device device = {KEY_DEVICE, 0, 100, 1760640900, "1"};
uint32_t lastTimeDevice = 0;

CircularBuffer<int8_t, 256> buffer(myBuffer);

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
  // if (port->value) scriptRunner.addScript(10, "13:*200/30", RESTART);
  // Serial.println(infoFS.uptime);
  if (port->value) scriptRunner.addScript(10, "if:$p13==0,$p13=200/50,else,$p13=0/30,end", IGNORE);
}

void getADC() {
  device.analog = analogRead(A0);
}

void getData() {
  getADC();
}

bool readProvider(const char* id, DataType& type, uint32_t& value) {
  Serial.print("📖 readProvider: ");
  Serial.println(id);

  if (id[0] == '$' && id[1] == 'p' && isdigit(id[2])) {
    uint8_t gpio = atoi(id + 2);
    Serial.print("   GPIO: ");
    Serial.println(gpio);
    for (int i = 0; i < ports_len; i++) {
      if (ports[i].gpio == gpio) {
        type = DATA_INT;
        value = ports[i].value;
        Serial.print("   → value: ");
        Serial.println(value);
        return true;
      }
    }
    Serial.println("   ❌ Port not found!");
    return false;
  }

  Serial.println("   ❌ Not handled by readProvider");
  return false;
}

// ============================================================
// ===== WRITE PROVIDER =====
// ============================================================

bool writeProvider(const char* id, const char* value) {
  Serial.print("✍️ writeProvider: ");
  Serial.print(id);
  Serial.print(" = ");
  Serial.println(value);

  // Логируем, но ничего не делаем
  // Возвращаем false, чтобы ScriptRunner использовал встроенную логику
  return false;
}

void logProvider(const char* message) {
  Serial.println(message);

  // Можно отправить в WebSocket:
  // wsSendAll((uint8_t*)message, strlen(message));

  // Можно сохранить в файл:
  // appendToFile("/log.txt", message);
}

void stateChangeProvider(uint8_t gpio, uint16_t oldValue, uint16_t newValue) {
  updatePort(gpio, newValue);
}

void onPortOutput(uint8_t gpio, uint16_t value, uint8_t portType) {
  setValue(gpio, value);
}

void setupDevice() {
  // scriptRunner.addScript(1, "[5]14:1,p5,14:0,p5]", RESTART);
  // scriptRunner.addDataSource("uptime", DATA_UINT32, (void*)&infoFS.uptime);
  scriptRunner.setReadProvider(readProvider);
  scriptRunner.setWriteProvider(writeProvider);
  scriptRunner.setLogProvider(logProvider);
  scriptRunner.setStateChangeProvider(stateChangeProvider);
  scriptRunner.setPortOutputCallback(onPortOutput);
  // scriptRunner.addScript(2, "[*]14:1,wait:5s,14:0,wait:5s]", RESTART);
  // scriptRunner.addScript(8, "[*]$p14=1,p50,$p14=0,p50]", RESTART);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getData();
    onSendDevice();

    buffer.push(random(-55, 55));
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
