#include "./include/device.h"

#include "./include/gpio.h"
#include "./include/init.h"
#include "./include/tasks.h"

bool gReverseDirection = false;

struct ModeType {
  uint8_t Brightness = 10U;
  uint8_t Speed = 30U;
  uint8_t Scale = 40U;
};

CRGB leds[NUM_LEDS];
CRGBPalette16 gPal;
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
      Serial.println("fgghfgfgfg");
      Serial.println(len);
      Serial.println(info->len);
      Serial.println(info->index);
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

void led() {
  // Serial.println("test");
}

void setupDevice() {
  FastLED.addLeds<CHIPSET, LED_PIN, COLOR_ORDER>(leds, NUM_LEDS);  // .setCorrection(TypicalLEDStrip);
  FastLED.setBrightness(BRIGHTNESS);

  FastLED.setMaxPowerInVoltsAndMilliamps(5, CURRENT_LIMIT);

  gPal = HeatColors_p;
  FastLED.clear();
  FastLED.show();
  led();
  // Serial.println("test");
}

static uint16_t x;
static uint16_t y;
static uint16_t z;

uint16_t speed = 20;  // speed is set dynamically once we've started up
uint16_t scale = 30;  // scale is set dynamically once we've started up

// This is the array that we keep our computed noise values in
#define MAX_DIMENSION (max(WIDTH, HEIGHT))
#if (WIDTH > HEIGHT)
uint8_t noise[WIDTH][WIDTH];
#else
uint8_t noise[HEIGHT][HEIGHT];
#endif

CRGBPalette16 currentPalette(PartyColors_p);
uint8_t colorLoop = 1;
uint8_t ihue = 0;

static const uint8_t valueMask[8][16] PROGMEM =
    {
        {32, 0, 0, 0, 0, 0, 0, 32, 32, 0, 0, 0, 0, 0, 0, 32},
        {64, 0, 0, 0, 0, 0, 0, 64, 64, 0, 0, 0, 0, 0, 0, 64},
        {96, 32, 0, 0, 0, 0, 32, 96, 96, 32, 0, 0, 0, 0, 32, 96},
        {128, 64, 32, 0, 0, 32, 64, 128, 128, 64, 32, 0, 0, 32, 64, 128},
        {160, 96, 64, 32, 32, 64, 96, 160, 160, 96, 64, 32, 32, 64, 96, 160},
        {192, 128, 96, 64, 64, 96, 128, 192, 192, 128, 96, 64, 64, 96, 128, 192},
        {255, 160, 128, 96, 96, 128, 160, 255, 255, 160, 128, 96, 96, 128, 160, 255},
        {255, 192, 160, 128, 128, 160, 192, 255, 255, 192, 160, 128, 128, 160, 192, 255}};

static const uint8_t hueMask[8][16] PROGMEM =
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

// ************* НАСТРОЙКА МАТРИЦЫ *****
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

void fillAll(CRGB color) {
  for (int32_t i = 0; i < NUM_LEDS; i++) {
    leds[i] = color;
  }
}

// функция отрисовки точки по координатам X Y
void drawPixelXY(int16_t x, int16_t y, CRGB color) {
  if (x < 0 || x > (int16_t)(WIDTH - 1) || y < 0 || y > (int16_t)(HEIGHT - 1)) return;
  uint32_t thisPixel = getPixelNumber((uint8_t)x, (uint8_t)y) * SEGMENTS;
  for (uint8_t i = 0; i < SEGMENTS; i++) {
    leds[thisPixel + i] = color;
  }
}

// функция получения цвета пикселя по его номеру
uint32_t getPixColor(uint32_t thisSegm) {
  uint32_t thisPixel = thisSegm * SEGMENTS;
  if (thisPixel > NUM_LEDS - 1) return 0;
  return (((uint32_t)leds[thisPixel].r << 16) | ((uint32_t)leds[thisPixel].g << 8) | (uint32_t)leds[thisPixel].b);
}

// функция получения цвета пикселя в матрице по его координатам
uint32_t getPixColorXY(uint8_t x, uint8_t y) {
  return getPixColor(getPixelNumber(x, y));
}

void drawFrame(uint8_t pcnt, bool isColored) {
  int32_t nextv;

  // each row interpolates with the one before it
  for (uint8_t y = HEIGHT - 1U; y > 0U; y--) {
    for (uint8_t x = 0U; x < WIDTH; x++) {
      uint8_t newX = x;
      if (x > 15U) newX = x % 16U;
      if (y < 8U) {
        nextv =
            (((100.0 - pcnt) * matrixValue[y][newX] + pcnt * matrixValue[y - 1][newX]) / 100.0) - pgm_read_byte(&valueMask[y][newX]);

        CRGB color = CHSV(
            isColored ? modes[EFF_FIRE].Scale * 2.5 + pgm_read_byte(&hueMask[y][newX]) : 0U,  // H
            isColored ? 255U : 0U,                                                            // S
            (uint8_t)max(0, nextv)                                                            // V
        );

        leds[getPixelNumber(x, y)] = color;
      } else if (y == 8U && SPARKLES) {
        if (random(0, 20) == 0 && getPixColorXY(x, y - 1U) != 0U)
          drawPixelXY(x, y, getPixColorXY(x, y - 1U));
        else
          drawPixelXY(x, y, 0U);
      } else if (SPARKLES) {
        // старая версия для яркости
        if (getPixColorXY(x, y - 1U) > 0U)
          drawPixelXY(x, y, getPixColorXY(x, y - 1U));
        else
          drawPixelXY(x, y, 0U);
      }
    }
  }

  // first row interpolates with the "next" line
  for (uint8_t x = 0U; x < WIDTH; x++) {
    uint8_t newX = x;
    if (x > 15U) newX = x % 16U;
    CRGB color = CHSV(
        isColored ? modes[EFF_FIRE].Scale * 2.5 + pgm_read_byte(&(hueMask[0][newX])) : 0U,  // H
        isColored ? 255U : 0U,                                                              // S
        (uint8_t)(((100.0 - pcnt) * matrixValue[0][newX] + pcnt * line[newX]) / 100.0)      // V
    );
    // leds[getPixelNumber(newX, 0)] = color;                                         // на форуме пишут что это ошибка - вместо newX должно быть x, иначе
    leds[getPixelNumber(x, 0)] = color;  // на матрицах шире 16 столбцов нижний правый угол неработает
  }
}

void fireRoutine(bool isColored)  // true - цветной огонь, false - белый
{
  if (loadingFlag) {
    loadingFlag = false;
    FastLED.clear();
    generateLine();
    memset(matrixValue, 0, sizeof(matrixValue));
  }
  if (pcnt >= 100) {
    shiftUp();
    generateLine();
    pcnt = 0;
  }
  drawFrame(pcnt, isColored);
  pcnt += 30;
}

void fillNoiseLED() {
  uint8_t dataSmoothing = 0;
  if (speed < 50) {
    dataSmoothing = 200 - (speed * 4);
  }
  for (uint8_t i = 0; i < MAX_DIMENSION; i++) {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0; j < MAX_DIMENSION; j++) {
      int32_t joffset = scale * j;

      uint8_t data = inoise8(x + ioffset, y + joffset, z);

      data = qsub8(data, 16);
      data = qadd8(data, scale8(data, 39));

      if (dataSmoothing) {
        uint8_t olddata = noise[i][j];
        uint8_t newdata = scale8(olddata, dataSmoothing) + scale8(data, 256 - dataSmoothing);
        data = newdata;
      }

      noise[i][j] = data;
    }
  }
  z += speed;

  // apply slow drift to X and Y, just for visual variation.
  x += speed / 8;
  y -= speed / 16;

  for (uint8_t i = 0; i < WIDTH; i++) {
    for (uint8_t j = 0; j < HEIGHT; j++) {
      uint8_t index = noise[j][i];
      uint8_t bri = noise[i][j];
      // if this palette is a 'loop', add a slowly-changing base value
      if (colorLoop) {
        index += ihue;
      }
      // brighten up, as the color palette itself often contains the
      // light/dark dynamic range desired
      if (bri > 127) {
        bri = 255;
      } else {
        bri = dim8_raw(bri * 2);
      }
      CRGB color = ColorFromPalette(currentPalette, index, bri);
      drawPixelXY(i, j, color);  // leds[getPixelNumber(i, j)] = color;
    }
  }
  ihue += 1;
}

void fillnoise8() {
  for (uint8_t i = 0; i < MAX_DIMENSION; i++) {
    int32_t ioffset = scale * i;
    for (uint8_t j = 0; j < MAX_DIMENSION; j++) {
      int32_t joffset = scale * j;
      noise[i][j] = inoise8(x + ioffset, y + joffset, z);
    }
  }
  z += speed;
}

void madnessNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    scale = modes[EFF_MADNESS].Scale;
    speed = modes[EFF_MADNESS].Speed;
  }
  fillnoise8();
  for (uint8_t i = 0; i < WIDTH; i++) {
    for (uint8_t j = 0; j < HEIGHT; j++) {
      CRGB thisColor = CHSV(noise[j][i], 255, noise[i][j]);
      drawPixelXY(i, j, thisColor);  // leds[getPixelNumber(i, j)] = CHSV(noise[j][i], 255, noise[i][j]);
    }
  }
  ihue += 1;
}

void rainbowNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = RainbowColors_p;
    scale = modes[EFF_RAINBOW].Scale;
    speed = modes[EFF_RAINBOW].Speed;
    colorLoop = 1;
  }
  fillNoiseLED();
}

void rainbowStripeNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = RainbowStripeColors_p;
    scale = modes[EFF_RAINBOW_STRIPE].Scale;
    speed = modes[EFF_RAINBOW_STRIPE].Speed;
    colorLoop = 1;
  }
  fillNoiseLED();
}

void zebraNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    // 'black out' all 16 palette entries...
    fill_solid(currentPalette, 16, CRGB::Black);
    // and set every fourth one to white.
    currentPalette[0] = CRGB::White;
    currentPalette[4] = CRGB::White;
    currentPalette[8] = CRGB::White;
    currentPalette[12] = CRGB::White;
    scale = modes[EFF_ZEBRA].Scale;
    speed = modes[EFF_ZEBRA].Speed;
    colorLoop = 1;
  }
  fillNoiseLED();
}

void forestNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = ForestColors_p;
    scale = modes[EFF_FOREST].Scale;
    speed = modes[EFF_FOREST].Speed;
    colorLoop = 0;
  }
  fillNoiseLED();
}

void oceanNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = OceanColors_p;
    scale = modes[EFF_OCEAN].Scale;
    speed = modes[EFF_OCEAN].Speed;
    colorLoop = 0;
  }

  fillNoiseLED();
}

void plasmaNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = PartyColors_p;
    scale = modes[EFF_PLASMA].Scale;
    speed = modes[EFF_PLASMA].Speed;
    colorLoop = 1;
  }
  fillNoiseLED();
}

void cloudsNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = CloudColors_p;
    scale = modes[EFF_CLOUDS].Scale;
    speed = modes[EFF_CLOUDS].Speed;
    colorLoop = 0;
  }
  fillNoiseLED();
}

void lavaNoiseRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    currentPalette = LavaColors_p;
    scale = modes[EFF_LAVA].Scale;
    speed = modes[EFF_LAVA].Speed;
    colorLoop = 0;
  }
  fillNoiseLED();
}

// ************* СЛУЖЕБНЫЕ *************
#define LIGHTERS_AM (100U)
int32_t lightersPos[2U][LIGHTERS_AM];
int8_t lightersSpeed[2U][LIGHTERS_AM];
CHSV lightersColor[LIGHTERS_AM];
uint8_t loopCounter;
int32_t angle[LIGHTERS_AM];
int32_t speedV[LIGHTERS_AM];
int8_t angleSpeed[LIGHTERS_AM];
void lightersRoutine() {
  if (loadingFlag) {
    loadingFlag = false;
    randomSeed(millis());
    for (uint8_t i = 0U; i < LIGHTERS_AM; i++) {
      lightersPos[0U][i] = random(0, WIDTH * 10);
      lightersPos[1U][i] = random(0, HEIGHT * 10);
      lightersSpeed[0U][i] = random(-10, 10);
      lightersSpeed[1U][i] = random(-10, 10);
      lightersColor[i] = CHSV(random(0U, 255U), 255U, 255U);
    }
  }
  FastLED.clear();
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

    fireRoutine(true);
    // lavaNoiseRoutine();
    // lightersRoutine();
    FastLED.show();
  }
}

void loopDevice(uint32_t now) {
  effectsTick(now);

  // if (now - lastTimeDevice > 10000) {
  //   lastTimeDevice = now;

  //   send((uint8_t *)&device, sizeof(device), KEY_DEVICE);
  // }
  // if (tasks[KEY_DEVICE]) {
  //   // onSend(device.direction);
  //   // Serial.println("test");
  //   led();
  //   tasks[KEY_DEVICE] = 0;
  // };
}
