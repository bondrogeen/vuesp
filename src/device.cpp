#include "./include/device.h"

#include <Wire.h>

#include "./include/UnixTime.h"
#include "./include/files.h"
#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/runner.h"
#include "./include/tasks.h"
#include "./libs/CircularBuffer.cpp"

ScriptRunner scriptRunner;

extern Port ports[];
extern int ports_len;

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
  Serial.print(port->gpio);
  Serial.println(port->value);
  // scriptRunner.addScript(3, "13:255,p50,13:0,p50", IGNORE);
  Serial.println(infoFS.uptime);
  scriptRunner.addScript(10, "if:?:uptime<20,13:250,p50,13:0,else,13:250,p10,13:0,end", IGNORE);
}

void getADC() {
  device.analog = analogRead(A0);
}

void getData() {
  getADC();
}

void setupDevice() {
  // scriptRunner.addScript(1, "[5]13:1,p20,13:0]", RESTART);
  scriptRunner.initPorts(ports, ports_len);
  scriptRunner.addDataSource("uptime", DATA_UINT32, (void*)&infoFS.uptime);
  // scriptRunner.addScript(2, "[*]14:1,p50,14:0,p50]", RESTART);
}

void setupFirstDevice() {
  getLoadDef(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
}

void loopDevice(uint32_t now) {
  scriptRunner.update();

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
