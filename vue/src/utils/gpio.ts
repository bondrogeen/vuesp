export const MODE = {
  INPUT: 0,
  OUTPUT: 1,
  INPUT_PULLUP: 2,
  OUTPUT_OPEN_DRAIN: 3,
  PWM: 128,
  ONEWIRE: 129,
};

export const COMMAND = {
  GPIO_COMMAND_SAVE: 254,
  GPIO_COMMAND_GET: 1,
  GPIO_COMMAND_SET: 2,
  GPIO_COMMAND_GET_ALL: 3,
  GPIO_COMMAND_CHANGE: 4,
  GPIO_COMMAND_RESTORE: 5,
};

export const getBit = (byte: number, mask: number) => (byte & mask ? 1 : 0);
export const setBit = (byte: number, mask: number) => (byte |= mask);
export const clearBit = (byte: number, mask: number) => (byte &= ~mask);
export const toggleBit = (byte: number, mask: number) => (byte ^= mask);
