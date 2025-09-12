export const mask = {
  GPIO_VALUE: 0b00000001,
  GPIO_STATUS: 0b00000010,
  GPIO_INTERRUPT: 0b00001100,
  GPIO_MODE: 0b01110000,
  GPIO_DISABLED: 0b10000000,
};

export const command = {
  GPIO_COMMAND_GET: 0,
  GPIO_COMMAND_SET: 1,
  GPIO_COMMAND_GET_ALL: 2,
};

export const parseDateGPIO = (array: any): ArrayBuffer[] => {
  const arr: ArrayBuffer[] = [];
  const length = 8; // Port length
  for (let i = 0; i < array.length; i += length) {
    const data = array.slice(i, i + length);
    const uint8View = new Uint8Array(data);
    arr.push(uint8View.buffer);
  }
  return arr;
};

export const stringifyDateGPIO = (object: any) => {
  const arr = [];

  for (const key in object) {
    arr.push(object[key].gpio);
    arr.push(object[key].data);
  }
  return arr;
};

export const getBit = (byte: number, mask: number) => (byte & mask ? 1 : 0);

export const setBit = (byte: number, mask: number) => (byte |= mask);

export const clearBit = (byte: number, mask: number) => (byte &= ~mask);

export const toggleBit = (byte: number, mask: number) => (byte ^= mask);

export const getData = (byte: number): any => {
  return {
    disabled: (byte & mask.GPIO_DISABLED) >> 7,
    mode: (byte & mask.GPIO_MODE) >> 4,
    interrupt: (byte & mask.GPIO_INTERRUPT) >> 2,
    status: (byte & mask.GPIO_STATUS) >> 1,
    value: (byte & mask.GPIO_VALUE) >> 0,
  };
};

export const setData = (obj: any) => {
  let data = 0;
  data = obj.value ? setBit(data, mask.GPIO_VALUE) : clearBit(data, mask.GPIO_VALUE);
  data = obj.disabled ? setBit(data, mask.GPIO_DISABLED) : clearBit(data, mask.GPIO_DISABLED);
  data = obj.status ? setBit(data, mask.GPIO_STATUS) : clearBit(data, mask.GPIO_STATUS);
  data = clearBit(data, mask.GPIO_MODE);
  data |= (obj.mode & 0b111) << 4;
  return data;
};

export const getKey = (byte: number, key: string) => {
  return getData(byte)?.[key];
};

// export const setData = byte => {
//   return {
//     value: getBit(byte, mask.GPIO_VALUE),
//     mode: getBit(byte, mask.GPIO_MODE),
//     disabled: getBit(byte, mask.GPIO_DISABLED),
//     status: getBit(byte, mask.GPIO_STATUS),
//   };
// };

export const getValue = (data: number) => {
  return Boolean(data | mask.GPIO_VALUE);
};

// export const getValue = () => {

// }
