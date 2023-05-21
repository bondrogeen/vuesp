export const mask = {
  GPIO_INIT: 1,
  GPIO_EMPTY_2: 2,
  GPIO_EMPTY_3: 4,
  GPIO_EMPTY_4: 8,
  GPIO_EMPTY_5: 16,
  GPIO_VALUE: 32,
  GPIO_MODE: 64,
  GPIO_STATUS: 128,
};

export const command = {
  GPIO_COMMAND_GET: 0,
  GPIO_COMMAND_SET: 1,
  GPIO_COMMAND_GET_ALL: 2,
};

export const parseDateGPIO = array => {
  const arr = [];
  for (let i = 0; i < array.length; i += 2) {
    arr.push({ gpio: array[i], data: array[i + 1] });
  }
  return arr;
};

export const stringifyDateGPIO = object => {
  const arr = [];

  for (const key in object) {
    console.log(object[key]);
    arr.push(object[key].gpio);
    arr.push(object[key].data);
  }
  return arr;
};

export const getBit = (byte, mask) => (byte & mask ? 1 : 0);

export const setBit = (byte, mask) => (byte |= mask);

export const clearBit = (byte, mask) => (byte &= ~mask);

export const toggleBit = (byte, mask) => (byte ^= mask);

export const getData = byte => {
  return {
    value: getBit(byte, mask.GPIO_VALUE),
    mode: getBit(byte, mask.GPIO_MODE),
    init: getBit(byte, mask.GPIO_INIT),
    status: getBit(byte, mask.GPIO_STATUS),
  };
};

export const setData = obj => {
  let data = 0;
  data = obj.value ? setBit(data, mask.GPIO_VALUE) : clearBit(data, mask.GPIO_VALUE);
  data = obj.mode ? setBit(data, mask.GPIO_MODE) : clearBit(data, mask.GPIO_MODE);
  data = obj.init ? setBit(data, mask.GPIO_INIT) : clearBit(data, mask.GPIO_INIT);
  data = obj.status ? setBit(data, mask.GPIO_STATUS) : clearBit(data, mask.GPIO_STATUS);
  return data;
};

export const getKey = (byte, key) => {
  return getData(byte)?.[key];
};

// export const setData = byte => {
//   return {
//     value: getBit(byte, mask.GPIO_VALUE),
//     mode: getBit(byte, mask.GPIO_MODE),
//     init: getBit(byte, mask.GPIO_INIT),
//     status: getBit(byte, mask.GPIO_STATUS),
//   };
// };

export const getValue = data => {
  return Boolean(data | mask.GPIO_VALUE);
};

// export const getValue = () => {

// }
