#include "./include/led.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(256, PIN, NEO_GRB + NEO_KHZ800);

uint32_t lastTimeLed = 0;
uint8_t test = 0;

uint8_t buffer[1024] = {255, 0, 0, 0, 128};

void setupLed() {
  strip.begin();
  strip.setBrightness(5);
  strip.show();
}

void print() {
  // for (uint16_t i = 0; i < 1024; i++) {
  //   Serial.print(buffer[i]);
  // }
  //   Serial.println("");
  for (uint16_t i = 0; i <= 256; i++) {
    uint32_t index = i * 4;
    strip.setPixelColor(i, buffer[index], buffer[index + 1], buffer[index + 2], buffer[index + 3]);
    // strip.setPixelColor(i, 0, 0, 255, 255);
  }
  Serial.println("");
  strip.show();
}

void loopLed(uint32_t now) {
  if (now - lastTimeLed > 5000) {
    // Serial.println("test");
    lastTimeLed = now;
    // colorWipe(strip.Color(255, 0, 0), 50);  // Red
    // Serial.println(strip.Color(128, 0, 255), HEX);
    // test++;
    File file = LittleFS.open("/test.txt", "r");
    if (!file) {
      Serial.println("file open failed");
    } else {
      Serial.println("file open");
      while (file.available()) {
        file.read(buffer, 1024);
      }
      file.close();
      // Serial.print(buffer);
      print();
    }
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