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
  if (port->value) scriptRunner.addScript(10, "if:?:uptime<20,13:*200/30,else,13:*200/10,end", IGNORE);
}

void getADC() {
  device.analog = analogRead(A0);
}

void getData() {
  getADC();
}

bool dataProvider(const char* id, DataType& type, uint32_t& value) {
  // Системные
  Serial.println(id);
  if (strcmp(id, "uptime") == 0) {
    type = DATA_UINT32;
    value = infoFS.uptime;
    return true;
  }

  // if (strcmp(id, "heap") == 0) {
  //   type = DATA_UINT32;
  //   value = ESP.getFreeHeap();
  //   return true;
  // }

  // // Устройство
  // if (strcmp(id, "temp") == 0) {
  //   type = DATA_FLOAT;
  //   value = (uint32_t)(device.temperature * 10);
  //   return true;
  // }

  // if (strcmp(id, "analog") == 0) {
  //   type = DATA_INT;
  //   value = device.analog;
  //   return true;
  // }

  // Dallas датчики
  // if (strncmp(id, "ds_", 3) == 0) {
  //   type = DATA_FLOAT;
  //   float temp = readDallasTemperature(id + 3);
  //   if (temp < -50) return false;
  //   value = (uint32_t)(temp * 10);
  //   return true;
  // }

  return false;
}

void logProvider(const char* message) {
  Serial.println(message);

  // Можно отправить в WebSocket:
  // wsSendAll((uint8_t*)message, strlen(message));

  // Можно сохранить в файл:
  // appendToFile("/log.txt", message);
}

void myStateChangeProvider(uint8_t gpio, uint16_t oldValue, uint16_t newValue) {
  Serial.print("🔔 GPIO ");
  Serial.print(gpio);
  Serial.print(" changed from ");
  Serial.print(oldValue);
  Serial.print(" to ");
  Serial.println(newValue);

  for (int i = 0; i < ports_len; i++) {
    if (ports[i].gpio == gpio) {
      ports[i].value = newValue;
      break;
    }
  }
  getAll();
}

void setupDevice() {
  // scriptRunner.addScript(1, "[5]13:1,p20,13:0]", RESTART);
  scriptRunner.initPorts(ports, ports_len);
  // scriptRunner.addDataSource("uptime", DATA_UINT32, (void*)&infoFS.uptime);
  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.setLogProvider(logProvider);
  scriptRunner.setStateChangeProvider(myStateChangeProvider);
  scriptRunner.addScript(2, "[*]14:1,log:Temperature: ?:uptime,wait:5s,14:0,p50]", RESTART);
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
