export const getBit = (byte, mask) => (byte & mask ? 1 : 0)

export const setBit = (byte, mask) => (byte |= mask);

export const clearBit = (byte, mask) => (byte &= ~mask);

const getValue = (name = '', obj = {}) => {

  // console.log(name, obj);

  const keys = name.split('.');
  let current = obj;
  for (const key of keys) {
    if (current !== null && typeof current === 'object' && key in current) {
      current = current[key];
    } else {
      return undefined;
    }
  }
  return current;
}

export const list = [
  { name: 'device.input.1', key: 'device.input', get: (byte) => getBit(byte, 0b00000001) },
  { name: 'device.input.2', key: 'device.input', get: (byte) => getBit(byte, 0b00000010) },
  { name: 'device.input.3', key: 'device.input', get: (byte) => getBit(byte, 0b00000100) },
  { name: 'device.input.4', key: 'device.input', get: (byte) => getBit(byte, 0b00001000) },
  { name: 'device.input.5', key: 'device.input', get: (byte) => getBit(byte, 0b00010000) },
  { name: 'device.input.6', key: 'device.input', get: (byte) => getBit(byte, 0b00100000) },
  { name: 'device.output.1', key: 'device.output', get: (byte) => getBit(byte, 0b00000001), set: (byte, value) => !value ? clearBit(byte, 0b00000001) : setBit(byte, 0b00000001) },
  { name: 'device.output.2', key: 'device.output', get: (byte) => getBit(byte, 0b00000010), set: (byte, value) => !value ? clearBit(byte, 0b00000010) : setBit(byte, 0b00000010) },
  { name: 'device.output.3', key: 'device.output', get: (byte) => getBit(byte, 0b00000100), set: (byte, value) => !value ? clearBit(byte, 0b00000100) : setBit(byte, 0b00000100) },
  { name: 'device.output.4', key: 'device.output', get: (byte) => getBit(byte, 0b00001000), set: (byte, value) => !value ? clearBit(byte, 0b00001000) : setBit(byte, 0b00001000) },
  { name: 'device.output.5', key: 'device.output', get: (byte) => getBit(byte, 0b00010000), set: (byte, value) => !value ? clearBit(byte, 0b00010000) : setBit(byte, 0b00010000) },
  { name: 'device.output.6', key: 'device.output', get: (byte) => getBit(byte, 0b00100000), set: (byte, value) => !value ? clearBit(byte, 0b00100000) : setBit(byte, 0b00100000) },
  { name: 'device.adc.1', key: 'device.adc1', },
  { name: 'device.adc.2', key: 'device.adc2', },
  { name: 'device.adc.3', key: 'device.adc3', },
  { name: 'device.adc.4', key: 'device.adc4', },
  { name: 'device.dac.1', key: 'device.dac1', },
  { name: 'device.now', key: 'device.now', },
]



class Device {
  #name
  #key
  #get
  #set
  constructor({ name, key, get, set }) {
    this.#name = name
    this.#key = key
    this.#get = get
    this.#set = set
  }

  get(object) {
    const value = getValue(this.#key, object)
    // console.log(value);

    if (this.#get) {
      return this.#get(value)
    }
    return value
  }

  set(object, value) {
    const byte = getValue(this.#key, object)
    if (this.#set) {
      return this.#set(byte, value)
    }
    return null
  }

}

class Devices {
  #list
  constructor(list) {
    // console.log(list);

    this.#list = new Map()
    for (let i = 0; i < list.length; i++) {
      const item = list[i]
      this.#list.set(item.name, new Device(item))
    }

  }



  get(name, object) {
    const data = this.#list.get(name)
    // console.log(data);
    return data ? data.get(object) : null

    // return this.getValue(name, object)
  }

  set(name, object, value) {
    const data = this.#list.get(name)
    // console.log(data);
    return data ? data.set(object, value) : null

    // return this.getValue(name, object)
  }

  getList() {
    const list = []
    for (let [key, elem] of this.#list) {
      list.push(key)
    }
    return list
  }
}

export const data = new Devices(list)
