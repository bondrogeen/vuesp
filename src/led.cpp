#include "led.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(256, PIN, NEO_GRB + NEO_KHZ800);

uint32_t lastTimeLed = 0;
uint8_t test = 0;

void setupLed() {
  strip.begin();
  strip.setBrightness(5);
  strip.show();
}

void loopLed(uint32_t now) {
  if (now - lastTimeLed > 1000) {
    Serial.println("test");
    lastTimeLed = now;
    colorWipe(strip.Color(255, 0, 0), 50);  // Red
    Serial.println(strip.Color(128, 0, 255), HEX);
    test++;
  }
}

void colorWipe(uint32_t c, uint8_t wait) {
  for (uint16_t i = 0; i < strip.numPixels(); i++) {
    if (test > i) {
      strip.setPixelColor(i, c);
    } else {
      strip.setPixelColor(i, 0);
    }
  }
  strip.show();
}