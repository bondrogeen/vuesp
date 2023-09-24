#include "./include/device.h"

#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(256, D5, NEO_GRB + NEO_KHZ800);

struct ModeType {
  uint8_t Brightness = 10U;
  uint8_t Speed = 30U;
  uint8_t Scale = 40U;
};

// CRGB leds[NUM_LEDS];
// CRGBPalette16 gPal;
int8_t currentMode = 0;
uint32_t effTimer;
bool loadingFlag = true;
unsigned char matrixValue[8][16];

uint32_t lastTimeDevice = 0;
ModeType modes[MODE_AMOUNT];

#define SPARKLES (1U)  // вылетающие угольки вкл выкл
uint8_t line[WIDTH];
uint8_t pcnt = 0U;

Device device = {KEY_DEVICE, 42, 57, {0xffffffff, 0xff0000ff, 0x00ff00ff, 0x0000ffff, 0x000000ff, 0x00000000, 0x000000ff, 0x0000ff00, 0xffff0000, 5}};

uint8_t task;

void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId) {
  AwsFrameInfo *info = (AwsFrameInfo *)arg;

  if (info->opcode == WS_BINARY) {
    if (info->index == 0) {
      task = data[0];
    }
    if (info->len == sizeof(device)) {
      uint8_t *address = (uint8_t *)&device;
      for (size_t i = 0; i < len; i++) {
        *(address + i + info->index) = *(data + i);
      }
    }
    if ((info->index + len) == info->len) {
      tasks[task] = true;
    }
  }
}

void onSend(uint8_t direction) {
  device.example1 += 1;
  device.direction = direction;
  send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
}

const uint8_t valueMask[8][16] PROGMEM =
    {
        {32, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 32},
        {64, 0, 0, 0, 0, 0, 0, 64, 64, 0, 0, 0, 0, 0, 0, 64},
        {96, 32, 0, 0, 0, 0, 32, 96, 96, 32, 0, 0, 0, 0, 32, 96},
        {128, 64, 32, 0, 0, 32, 64, 128, 128, 64, 32, 0, 0, 32, 64, 128},
        {160, 96, 64, 32, 32, 64, 96, 160, 160, 96, 64, 32, 32, 64, 96, 160},
        {192, 128, 96, 64, 64, 96, 128, 192, 192, 128, 96, 64, 64, 96, 128, 192},
        {255, 160, 128, 96, 96, 128, 160, 255, 255, 160, 128, 96, 96, 128, 160, 255},
        {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}};

const uint8_t hueMask[8][16] PROGMEM =
    {
        {1, 11, 19, 25, 25, 22, 11, 1, 1, 11, 19, 25, 25, 22, 11, 1},
        {1, 8, 13, 19, 25, 19, 8, 1, 1, 8, 13, 19, 25, 19, 8, 1},
        {1, 8, 13, 16, 19, 16, 8, 1, 1, 8, 13, 16, 19, 16, 8, 1},
        {1, 5, 11, 13, 13, 13, 5, 1, 1, 5, 11, 13, 13, 13, 5, 1},
        {1, 5, 11, 11, 11, 11, 5, 1, 1, 5, 11, 11, 11, 11, 5, 1},
        {0, 1, 5, 8, 8, 5, 1, 0, 0, 1, 5, 8, 8, 5, 1, 0},
        {0, 0, 1, 5, 5, 1, 0, 0, 0, 0, 1, 5, 5, 1, 0, 0},
        {0, 0, 0, 1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0}};

void shiftUp() {
  for (uint8_t y = HEIGHT - 1U; y > 0U; y--) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15U) newX = x % 16U;
      if (y > 7U) continue;
      matrixValue[y][newX] = matrixValue[y - 1U][newX];
    }
  }

  for (uint8_t x = 0U; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15U) newX = x % 16U;
    matrixValue[0U][newX] = line[newX];
  }
}

void generateLine() {
  for (uint8_t x = 0U; x < WIDTH; x++) {
    line[x] = random(64, 255);
  }
}

// // ************* НАСТРОЙКА МАТРИЦЫ *****
#if (CONNECTION_ANGLE == 0 && STRIP_DIRECTION == 0)
#define _WIDTH WIDTH
#define THIS_X x
#define THIS_Y y

#endif

uint16_t getPixelNumber(uint8_t x, uint8_t y) {
  if ((THIS_Y % 2 == 0) || MATRIX_TYPE) {
    return (THIS_Y * _WIDTH + THIS_X);
  } else {
    return (THIS_Y * _WIDTH + _WIDTH - THIS_X - 1);
  }
}

void drawPixelXY(int16_t x, int16_t y, uint32_t color) {
  if (x < 0 || x > (int16_t)(WIDTH - 1) || y < 0 || y > (int16_t)(HEIGHT - 1)) return;
  uint32_t thisPixel = getPixelNumber((uint8_t)x, (uint8_t)y) * SEGMENTS;
  for (uint8_t i = 0; i < SEGMENTS; i++) {
    pixels.setPixelColor(thisPixel + i, color);
  }
}

uint32_t getPixColor(uint32_t thisSegm) {
  uint32_t thisPixel = thisSegm * SEGMENTS;
  if (thisPixel > NUM_LEDS - 1) return 0;
  return pixels.getPixelColor(thisPixel);
}

uint32_t getPixColorXY(uint8_t x, uint8_t y) {
  return getPixColor(getPixelNumber(x, y));
}

void drawFrame(uint8_t pcnt) {
  int32_t nextv;

  for (uint8_t y = HEIGHT - 1U; y > 0U; y--) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15U) newX = x % 16U;
      if (y < 8U) {
        nextv =
            (((100.0 - pcnt) * matrixValue[y][newX] + pcnt * matrixValue[y - 1][newX]) / 100.0) - pgm_read_byte(&valueMask[y][newX]);
        pixels.setPixelColor(getPixelNumber(x, y), (uint8_t)max(0, nextv), 0, 0);

      } else if (y == 8U && SPARKLES) {
        if (random(0, 20) == 0 && getPixColorXY(x, y - 1U) != 0U)
          drawPixelXY(x, y, getPixColorXY(x, y - 1U));
        else
          drawPixelXY(x, y, 0U);
      } else if (SPARKLES) {
        if (getPixColorXY(x, y - 1U) > 0U)
          drawPixelXY(x, y, getPixColorXY(x, y - 1U));
        else
          drawPixelXY(x, y, 0U);
      }
    }
  }

  for (uint8_t x = 0U; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15U) newX = x % 16U;
    pixels.setPixelColor(getPixelNumber(x, 0), (uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0), 0, 0);
  }
}

void fireRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    pixels.clear();
    generateLine();
    memset(matrixValue, 0, sizeof(matrixValue));
  }
  if (pcnt >= 100) {
    shiftUp();
    generateLine();
    pcnt = 0;
  }
  drawFrame(pcnt);
  pcnt += 30;
}

#define LIGHTERS_AM (100U)
int32_t lightersPos[2U][LIGHTERS_AM];
int8_t lightersSpeed[2U][LIGHTERS_AM];
uint32_t lightersColor[LIGHTERS_AM];
uint8_t loopCounter;

void lightersRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    randomSeed(millis());
    for (uint8_t i = 0U; i < LIGHTERS_AM; i++) {
      lightersPos[0U][i] = random(0, WIDTH * 10);
      lightersPos[1U][i] = random(0, HEIGHT * 10);
      lightersSpeed[0U][i] = random(-10, 10);
      lightersSpeed[1U][i] = random(-10, 10);
      lightersColor[i] = pixels.ColorHSV(random(0, 65000), 255, 255);
    }
  }
  pixels.clear();
  if (++loopCounter > 20U) loopCounter = 0U;
  for (uint8_t i = 0U; i < modes[EFF_LIGHTERS].Scale; i++) {
    if (loopCounter == 0U)  // меняем скорость каждые 255 отрисовок
    {
      lightersSpeed[0U][i] += random(-3, 4);
      lightersSpeed[1U][i] += random(-3, 4);
      lightersSpeed[0U][i] = constrain(lightersSpeed[0U][i], -20, 20);
      lightersSpeed[1U][i] = constrain(lightersSpeed[1U][i], -20, 20);
    }

    lightersPos[0U][i] += lightersSpeed[0U][i];
    lightersPos[1U][i] += lightersSpeed[1U][i];

    if (lightersPos[0U][i] < 0) lightersPos[0U][i] = (WIDTH - 1) * 10;
    if (lightersPos[0U][i] >= (int32_t)(WIDTH * 10)) lightersPos[0U][i] = 0;

    if (lightersPos[1U][i] < 0) {
      lightersPos[1U][i] = 0;
      lightersSpeed[1U][i] = -lightersSpeed[1U][i];
    }
    if (lightersPos[1U][i] >= (int32_t)(HEIGHT - 1) * 10) {
      lightersPos[1U][i] = (HEIGHT - 1U) * 10;
      lightersSpeed[1U][i] = -lightersSpeed[1U][i];
    }
    drawPixelXY(lightersPos[0U][i] / 10, lightersPos[1U][i] / 10, lightersColor[i]);
  }
}

void effectsTick(uint32_t now) {
  if (now - effTimer >= 60) {
    effTimer = now;

    fireRoutine();
    // lightersRoutine();
    pixels.show();
  }
}

void led() {
  pixels.clear();

  for (int i = 0; i < NUMPIXELS; i++) {
    uint8_t r = (uint8_t)(device.buffer[i] >> 24);
    uint8_t g = (uint8_t)(device.buffer[i] >> 16);
    uint8_t b = (uint8_t)(device.buffer[i] >> 8);
    pixels.setPixelColor(i, r, g, b);
    // Serial.println(device.buffer[i]);
  }
  pixels.show();
}

void setupDevice() {
  pixels.begin();
  pixels.setBrightness(50);
  // Serial.println("test");
}

void loopDevice(uint32_t now) {
  effectsTick(now);

  // if (now - lastTimeDevice > 10000) {
  //   lastTimeDevice = now;

  //   send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
  // }
  if (tasks[KEY_DEVICE]) {
    led();
    tasks[KEY_DEVICE] = 0;
  };
}
