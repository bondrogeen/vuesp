#ifndef VUESP_GENERATED_GPIO_CONFIG_H
#define VUESP_GENERATED_GPIO_CONFIG_H

// AUTO-GENERATED from vuesp.config.yaml — do not edit by hand.
// Included by src/gpio/gpio.cpp after gpio.h.

struct PinOverride {
  uint8_t gpio;
  uint8_t mode;
  uint8_t interrupt;
  uint8_t disabled;
  uint8_t isButton;
  uint8_t hasValue;
  uint16_t value;
};

static const PinOverride kPinOverrides[1] = {{0, 0, 0, 0, 0, 0, 0}};
static const uint8_t kPinOverrideCount = 0;

#endif
