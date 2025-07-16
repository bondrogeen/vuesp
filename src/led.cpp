#include "./include/led.h"

bool loadingFlag = true;
unsigned char matrixValue[8][16];

uint8_t line[WIDTH];
uint8_t pcnt = 0U;

int32_t lightersPos[2U][LIGHTERS_AM];
int8_t lightersSpeed[2U][LIGHTERS_AM];
uint32_t lightersColor[LIGHTERS_AM];
uint8_t loopCounter;

Adafruit_NeoPixel pixels = Adafruit_NeoPixel(NUMPIXELS, GPIO_LED, NEO_GRB + NEO_KHZ800);

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

uint16_t getPixelNumber(uint8_t x, uint8_t y) {
  if ((y % 2 == 0) || MATRIX_TYPE) {
    return (y * WIDTH + x);
  } else {
    return (y * WIDTH + WIDTH - x - 1);
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
  if (thisPixel > NUMPIXELS - 1) return 0;
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

void ledEffectFire() {
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

void ledEffectLighters() {
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
  for (uint8_t i = 0U; i < 40U; i++) {
    if (loopCounter == 0U) {
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

uint8_t ihue = 0;
void rainbow_fade() {  //-m2-FADE ALL LEDS THROUGH HSV RAINBOW
  ihue++;
  if (ihue > 255) {
    ihue = 0;
  }
  for (int i = 0; i < NUMPIXELS; i++) {
    // leds[idex] = CHSV(ihue, thissat, 255);
    pixels.setPixelColor(i, ihue, ihue, ihue);
  }
}

void led(Draw draw) {
  pixels.clear();

  for (int i = 0; i < NUMPIXELS; i++) {
    uint8_t r = (uint8_t)(draw.buffer[i] >> 24);
    uint8_t g = (uint8_t)(draw.buffer[i] >> 16);
    uint8_t b = (uint8_t)(draw.buffer[i] >> 8);
    pixels.setPixelColor(i, r, g, b);
    // Serial.println(device.buffer[i]);
  }
  pixels.show();
}

void ledShow() {
  pixels.show();
}

void ledClear() {
  pixels.clear();
}

void ledBrightness(uint8_t brightness) {
  pixels.setBrightness(brightness);
}

void setupLed() {
  pixels.begin();
}