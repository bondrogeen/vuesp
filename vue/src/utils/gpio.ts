import type { IListItem } from '@/types';

export const MODE = {
  INPUT: 0,
  OUTPUT: 1,
  INPUT_PULLUP: 2,
  OUTPUT_OPEN_DRAIN: 3,
};

export const COMMAND = {
  GPIO_COMMAND_GET: 0,
  GPIO_COMMAND_SET: 1,
  GPIO_COMMAND_GET_ALL: 2,
};

export const listMenu = [
  { name: 'Update', icon: 'IconUpdate' },
  { name: 'Save', icon: 'IconSave' },
];

export const listMode: IListItem[] = [
  { id: 1, name: 'INPUT', value: 0 }, // 0x00
  { id: 2, name: 'INPUT_PULLUP', value: 2 }, // 0x02
  { id: 3, name: 'OUTPUT', value: 1 }, // 0x01
  { id: 4, name: 'OUTPUT_OPEN_DRAIN', value: 3 }, // 0x03
  // { name: 'INPUT_PULLDOWN_16', value: 12 }, // 0x04
  // { name: 'WAKEUP_PULLUP', value: 13 }, // 0x05
  // { name: 'WAKEUP_PULLDOWN', value: 15 }, // 0x07
];

export const listInterrupt: IListItem[] = [
  { id: 1, name: 'OFF', value: 0 }, // 0x00
  { id: 2, name: 'RISING', value: 1 }, // 0x01
  { id: 3, name: 'FALLING', value: 2 }, // 0x02
  { id: 4, name: 'CHANGE', value: 3 }, // 0x03
];

export const getBit = (byte: number, mask: number) => (byte & mask ? 1 : 0);
export const setBit = (byte: number, mask: number) => (byte |= mask);
export const clearBit = (byte: number, mask: number) => (byte &= ~mask);
export const toggleBit = (byte: number, mask: number) => (byte ^= mask);
