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
  // if (port->value) scriptRunner.addScript(10, "13:*200/30", RESTART);
  // Serial.println(infoFS.uptime);
  if (port->value) {
    // scriptRunner.addScript(1, "$v0=255,log:v0=$v0,$v0=&15,log:AND 15=$v0,$v0=|240,log:OR 240=$v0,$v0=^255,log:XOR 255=$v0,$v0=~,log:NOT=$v0,$v0=1,log:v0=$v0,$v0=<<3,log:SHL 3=$v0,$v0=>>1,log:SHR 1=$v0", RESTART);

    scriptRunner.addScript(4, "$v0=($v0+2),$f4=($f4+2.4),log:Counter is $v0 $f4 $counter", RESTART);
    scriptRunner.addScript(9, "$counter+2,log:Counter is $counter,$s0='test',$s0+'tttt',$s0+25,$v1=153,$v1=@99,$s0+$v1,log:Length $s0#,log:String $s0 $v1", RESTART);
    scriptRunner.addScript(5, "if:$v0>=5&&$f4>16.7,call:2,end", RESTART);
  }
  // scriptRunner.addScript(4, "$v1=$p13,log:Port 13 is $v1", RESTART);
  if (port->value) {
    // scriptRunner.addScript(10, "if:$p13==0,$p13=200/20,else,$p13=0/10,end", RESTART);
    // scriptRunner.addScript(1, "$v0=0,if:$v0==1,log:SKIP_THIS,end,log:DONE", RESTART);
  }
}

void getADC() {
  device.analog = analogRead(A0);
}

void getData() {
  getADC();
}
static uint32_t counter = 0;
bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  Serial.print("📖 dataProvider: ");
  Serial.print(id);
  Serial.print(" kind: ");
  Serial.print(kind);
  Serial.print(" write: ");
  Serial.println(write);
  if (strcmp(id, "$counter") == 0) {
    if (kind == KIND_UINT) {
      if (write) {
        counter = value.uintVal;
      } else {
        value.uintVal = counter;
      }
      return true;
    }
  }

  return false;
}

void setupDevice() {
  // scriptRunner.addScript(1, "$v0=0,$f4=0", RESTART);
  scriptRunner.addScript(2, "[5]$p13=255,p5,$p13=0,p5]", RESTART);
  scriptRunner.setDataProvider(dataProvider);
  // scriptRunner.addScript(2, "[*]14:1,wait:5s,14:0,wait:5s]", RESTART);
  // scriptRunner.addScript(3, "[*]$p14=1,p50,$p14=0,p50]", RESTART);
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
