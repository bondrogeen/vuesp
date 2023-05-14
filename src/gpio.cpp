#include "./include/gpio.h"

#define GPIO_LED 13
#define GPIO_RELAY 14
#define GPIO_OUT_BTN 12
#define GPIO_IN_BTN 5

#define B_TRUE(bp, bb) (bp) |= (bb)
#define B_FALSE(bp, bb) (bp) &= ~(bb)
#define B_READ(bp, bb) bool((bp) & (bb))
// вот так храним наши флаги, значения обязательно как степени двойки!
#define GPIO_TEST 1
#define GPIO_STATUS 2
#define GPIO_MODE 4
#define GPIO_BUTTON_STATE 8
#define GPIO_INTERRUPT 16
#define GPIO_MODE_HIGT 32
#define GPIO_MODE_LOW 64
#define GPIO_ACTIVE 128

// active (on | off), state, mode

uint8_t defGPIO = 0b01010101;

Pins port = {
    KEY_PINS,
    1,
    2,
    4,
    16,
    defGPIO,
    defGPIO,
    defGPIO,
};

uint32_t lastTimeDevice = 0;
// uint8_t btnStatus = 0;
// Relay relay = {RELAY, 0, 0, 0};
// uint32_t debounce;

// void ICACHE_RAM_ATTR btnIsr() {
//   btnStatus = 1;
// }

// void sendStatus() {
//   uint8_t value = !digitalRead(GPIO_LED);
//   digitalWrite(GPIO_LED, value);
//   relay.status = value;
//   wsSend((uint8_t *)&relay, sizeof(relay));
//   attachInterrupt(GPIO_OUT_BTN, btnIsr, RISING);
//   attachInterrupt(GPIO_IN_BTN, btnIsr, FALLING);
// }

void setupGPIO() {
  pinMode(GPIO_LED, OUTPUT);
  pinMode(GPIO_RELAY, OUTPUT);
  // pinMode(GPIO_OUT_BTN, INPUT_PULLUP);
  // pinMode(GPIO_IN_BTN, INPUT_PULLUP);
  // attachInterrupt(GPIO_OUT_BTN, btnIsr, RISING);
  // attachInterrupt(GPIO_IN_BTN, btnIsr, FALLING);
}

void loopGPIO(uint32_t now) {
  // if (btnStatus) {
  //   btnStatus = 0;
  //   detachInterrupt(GPIO_OUT_BTN);
  //   detachInterrupt(GPIO_IN_BTN);
  //   Serial.println(btnStatus);
  //   sendStatus();
  // }

  if (now - lastTimeDevice > 1000) {
    lastTimeDevice = now;

    tasks[KEY_PINS] = 1;

    if (tasks[KEY_PINS]) send((uint8_t *)&port, sizeof(port), KEY_PINS);

    digitalWrite(GPIO_LED, !digitalRead(GPIO_LED));
    digitalWrite(GPIO_RELAY, !digitalRead(GPIO_RELAY));
    Serial.println("sdsdsds");
  }
}
