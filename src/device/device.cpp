#include "./device.h"

#include <Adafruit_AHTX0.h>
#include <Adafruit_BMP280.h>

Adafruit_AHTX0 aht;
Adafruit_BMP280 bmp;

sensors_event_t humidity, temp;

Device device = {KEY_DEVICE, DEVICE_COMMAND_GET_ALL, 0, 0, 0, "", SPEED, EFFECT_DRAW, BRIGHTNESS, 0, 0, 0};
Draw draw = {KEY_DRAW, DEVICE_COMMAND_GET_ALL, EFFECT_DRAW, BRIGHTNESS, {}};
Sensors sensors = {KEY_SENSORS};
Buffer myBuffer = {KEY_BUFFER};

uint8_t state;
uint32_t effTimer;
uint32_t lastTimeDevice = 0;

void onWsEventDevice(void* arg, uint8_t* data, size_t len, uint32_t clientId, uint8_t task) {
  if (!arg || !data || !len) return;

  AwsFrameInfo* info = (AwsFrameInfo*)arg;
  if (task == KEY_DEVICE && info->len == sizeof(device)) {
    memcpy(&device, data, sizeof(device));
  } else if (task == KEY_DRAW && info->len == sizeof(draw)) {
    memcpy(&draw, data, sizeof(draw));
  }
}

void onSendDevice() {
  wsSendAll((uint8_t*)&device, sizeof(device));
}

void sendDeviceTo(uint32_t clientId) {
  wsSendTo(clientId, (uint8_t*)&device, sizeof(device));
}

void deviceGPIO(Port* port, uint8_t type) {
  Serial.printf("gpio:%d, value:%d", port->gpio, port->value);
  if (type == EVENT_LONG_PRESS) {
    Serial.print(", type:long");
  } else if (type == EVENT_REPEAT) {
    Serial.print(", type:repeat");
  } else if (type == EVENT_CLICK) {
    Serial.printf(", type:click, count:%d", port->count);
    if (port->gpio == GPIO_BTN) {
      device.effect = device.effect >= EFFECT_CHASE ? EFFECT_DRAW : device.effect + 1;
      onSendDevice();
    }
  }
  Serial.println("");
}

static char displayBuffer[65] = "5";

bool dataProvider(const char* id, DataKind kind, DataValue& value, bool write) {
  Serial.print(id);
  Serial.print(write ? "WRITE" : "READ");
  Serial.print(" kind=");
  Serial.println(kind);

  if (strcmp(id, "$display") == 0) {
    if (write) {
      if (kind == KIND_STRING) {
        if (!value.stringVal.data) return false;
        char buf[65];
        uint8_t len = value.stringVal.len;
        if (len > 64) len = 64;
        strncpy(buf, (char*)value.stringVal.data, len);
        buf[len] = '\0';
        strncpy(displayBuffer, buf, sizeof(displayBuffer) - 1);
        displayBuffer[sizeof(displayBuffer) - 1] = '\0';
        Serial.print("DISPLAY: ");
        Serial.println(buf);
      } else if (kind == KIND_INT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%d", value.intVal);
      } else if (kind == KIND_UINT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%u", value.uintVal);
      } else if (kind == KIND_FLOAT) {
        snprintf(displayBuffer, sizeof(displayBuffer), "%.2f", value.floatVal);
      }
      return true;
    }

    if (kind == KIND_STRING) {
      value.stringVal.data = (uint8_t*)displayBuffer;
      value.stringVal.len = strlen(displayBuffer);
    } else if (kind == KIND_INT) {
      value.intVal = atoi(displayBuffer);
    } else if (kind == KIND_UINT) {
      value.uintVal = (uint32_t)atoi(displayBuffer);
    } else if (kind == KIND_FLOAT) {
      value.floatVal = atof(displayBuffer);
    }
    return true;
  }
  return false;
}

bool httpHandler(uint8_t paramCount, const Value* params, Value& result, void* userData) {
  if (!params || paramCount < 5) return false;
  if (!params[0].stringVal.data || !params[1].stringVal.data) return false;
  if (params[4].arrayVal.len > 0 && !params[4].arrayVal.data) return false;

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

void effectsTick(uint32_t now) {
  if (now - effTimer < device.speed) return;

  effTimer = now;
  state = 1;

  switch (device.effect) {
    case EFFECT_LIGHTERS:
      ledEffectLighters();
      break;
    case EFFECT_FIRE:
      ledEffectFire();
      break;
    case EFFECT_RAINBOW:
      rainbow_fade();
      break;
    case EFFECT_TWINKLING_STARS:
      twinklingStars();
      break;
    case EFFECT_CHASE:
      theaterChase();
      break;
    default:
      device.effect = 0;
      ledClear();
  }

  ledShow();
}

void getSensors() {
  sensors.status = aht.getEvent(&humidity, &temp);

  sensors.ahtTemperature = temp.temperature;
  sensors.ahtHumidity = humidity.relative_humidity;

  sensors.bmpTemperature = bmp.readTemperature();
  sensors.bmpPressure = bmp.readPressure();
  sensors.bmpAltitude = bmp.readAltitude(1013.25);
  wsSendAll((uint8_t*)&sensors, sizeof(sensors));
}

void setupDevice() {
  setupLed();
  ledBrightness(device.brightness);

  Wire.begin(GPIO_SDA, GPIO_SCL);
  bmp.begin();
  bmp.setSampling(Adafruit_BMP280::MODE_NORMAL, Adafruit_BMP280::SAMPLING_X2, Adafruit_BMP280::SAMPLING_X16, Adafruit_BMP280::FILTER_X16, Adafruit_BMP280::STANDBY_MS_500);
  aht.begin();

  scriptRunner.setDataProvider(dataProvider);
  scriptRunner.registerFunction("http", httpHandler);
}

void setupFirstDevice() {
  if (!loadConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device))) {
    saveConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
  }
  if (!loadConfigBlock(DEF_PATH_DRAW, (uint8_t*)&draw, sizeof(draw))) {
    saveConfigBlock(DEF_PATH_DRAW, (uint8_t*)&draw, sizeof(draw));
  }
}

void loopDevice(uint32_t now) {
  if (device.effect) {
    effectsTick(now);
  } else if (state) {
    state = 0;
    ledClear();
    ledShow();
  }

  device.button = digitalRead(GPIO_BTN);
  device.sensor = digitalRead(GPIO_HLK_LD101);

  if (now - lastTimeDevice > 10000) {
    lastTimeDevice = now;
    getSensors();
    onSendDevice();
  }

  if (tasks[KEY_BUFFER]) {
    wsSendAll((uint8_t*)&myBuffer, sizeof(myBuffer));
    tasks[KEY_BUFFER] = 0;
  }

  if (tasks[KEY_DEVICE]) {
    if (device.command == DEVICE_COMMAND_GET_ALL) {
      loadConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
      ledBrightness(device.brightness);
    }
    if (device.command == DEVICE_COMMAND_SET) {
      ledBrightness(device.brightness);
    }
    if (device.command == COMMAND_SAVE) {
      device.command = DEVICE_COMMAND_GET_ALL;
      saveConfigBlock(DEF_PATH_CONFIG, (uint8_t*)&device, sizeof(device));
    }
    onSendDevice();
    tasks[KEY_DEVICE] = 0;
  }

  if (tasks[KEY_SENSORS]) {
    getSensors();
    tasks[KEY_SENSORS] = 0;
  }

  if (tasks[KEY_DRAW]) {
    if (draw.command == DRAW_COMMAND_DRAW) {
      led(draw);
    }
    if (draw.command == DRAW_COMMAND_SAVE) {
      saveConfigBlock(DEF_PATH_DRAW, (uint8_t*)&draw, sizeof(draw));
    }
    wsSendAll((uint8_t*)&draw, sizeof(draw));
    tasks[KEY_DRAW] = 0;
  }
}
