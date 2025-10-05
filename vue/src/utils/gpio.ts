import type { IListItem } from '@/types';

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
};

export const listMenu: IListItem[] = [{ name: 'Save', value: 1 }];

export const listMode: IListItem[] = [
  { name: 'INPUT', value: MODE.INPUT }, 
  { name: 'INPUT_PULLUP', value: MODE.INPUT_PULLUP }, 
  { name: 'OUTPUT', value: MODE.OUTPUT }, 
  { name: 'OUTPUT_OPEN_DRAIN', value: MODE.OUTPUT_OPEN_DRAIN }, 
  { name: 'PWM', value: MODE.PWM },
  { name: 'ONEWIRE', value: MODE.ONEWIRE },
  // { name: 'INPUT_PULLDOWN_16', value: 12 }, // 0x04
  // { name: 'WAKEUP_PULLUP', value: 13 }, // 0x05
  // { name: 'WAKEUP_PULLDOWN', value: 15 }, // 0x07
];

export const listInterrupt: IListItem[] = [
  { name: 'OFF', value: 0 }, // 0x00
  { name: 'RISING', value: 1 }, // 0x01
  { name: 'FALLING', value: 2 }, // 0x02
  { name: 'CHANGE', value: 3 }, // 0x03
];

export const getBit = (byte: number, mask: number) => (byte & mask ? 1 : 0);
export const setBit = (byte: number, mask: number) => (byte |= mask);
export const clearBit = (byte: number, mask: number) => (byte &= ~mask);
export const toggleBit = (byte: number, mask: number) => (byte ^= mask);
