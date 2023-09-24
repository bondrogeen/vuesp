#ifndef DEVICE_H
#define DEVICE_H

#define NUMPIXELS 256
#include <FastLED.h>

#define LED_PIN D5
#define COLOR_ORDER GRB
#define CHIPSET WS2812B
#define NUM_LEDS 256

#define BRIGHTNESS 100
#define FRAMES_PER_SECOND 30
#define CURRENT_LIMIT (2000U)

#define MATRIX_TYPE (0U)
#define SEGMENTS (1U)
#define EFF_FIRE (1U)
#define MODE_AMOUNT (26U)

#define WIDTH (16U)
#define HEIGHT (16U)

#define EFF_SPARKLES (0U)         // Конфетти
#define EFF_FIRE (1U)             // Огонь
#define EFF_WHITTE_FIRE (2U)      // Белый огонь
#define EFF_RAINBOW_VER (3U)      // Радуга вертикальная
#define EFF_RAINBOW_HOR (4U)      // Радуга горизонтальная
#define EFF_RAINBOW_DIAG (5U)     // Радуга диагональная
#define EFF_COLORS (6U)           // Смена цвета
#define EFF_MADNESS (7U)          // Безумие 3D
#define EFF_CLOUDS (8U)           // Облака 3D
#define EFF_LAVA (9U)             // Лава 3D
#define EFF_PLASMA (10U)          // Плазма 3D
#define EFF_RAINBOW (11U)         // Радуга 3D
#define EFF_RAINBOW_STRIPE (12U)  // Павлин 3D
#define EFF_ZEBRA (13U)           // Зебра 3D
#define EFF_FOREST (14U)          // Лес 3D
#define EFF_OCEAN (15U)           // Океан 3D
#define EFF_COLOR (16U)           // Цвет
#define EFF_SNOW (17U)            // Снегопад
#define EFF_SNOWSTORM (18U)       // Метель
#define EFF_STARFALL (19U)        // Звездопад
#define EFF_MATRIX (20U)          // Матрица
#define EFF_LIGHTERS (21U)        // Светлячки
#define EFF_LIGHTER_TRACES (22U)  // Светлячки со шлейфом
#define EFF_PAINTBALL (23U)       // Пейнтбол
#define EFF_CUBE (24U)            // Блуждающий кубик
#define EFF_WHITE_COLOR (25U)     // Белый свет
#define MODE_AMOUNT (26U)

// #include <Adafruit_NeoPixel.h>
#include "./init.h"
#include "./tasks.h"
#include "./webserver.h"

void setupDevice();
void loopDevice(uint32_t now);
void onWsEventDevice(void *arg, uint8_t *data, size_t len, uint32_t clientId);

#endif