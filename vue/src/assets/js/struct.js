import Struct from 'c-struct-to-json';

class StructsToJSON {
  constructor() {
    this.keys = ['INIT'];
    this.isInit = false;
    this.onInit = null;
    this.structs = {};
    this.structs.INIT = new Struct([{ name: 'key', type: 'uint8_t' }]);
  }
  init(data) {
    if (typeof data === 'object') {
      const { keys, structs } = data;
      if (!keys && !Array.isArray(keys) && !structs && typeof data !== 'object') return;
      this.keys = keys;
      for (const key in structs) {
        const name = key.toUpperCase();
        const array = structs[key].map(({ t, n, l }) => ({ type: t, name: n, length: l }));
        this.structs[name] = new Struct(array);
      }
      this.isInit = true;
      if (this.onInit) this.onInit();
    }
  }

  parseBytesToJson(array) {
    try {
      let text = String.fromCharCode(...array);
      return JSON.parse(text);
    } catch (error) {
      console.warn(error);
    }
  }

  findIndexKey(name) {
    return this.keys.findIndex(i => i === name);
  }

  findNameKey(index) {
    return this.keys[index];
  }

  set(key, data) {
    const struct = this.structs[key];
    const index = this.findIndexKey(key);
    if (struct && data) {
      return struct.setObject({ ...data, key: index }).getBuffer();
    }
    if (typeof index !== 'undefined') return this.structs['INIT'].setObject({ key: index }).getBuffer();
    console.warn(`No struct or key ${key}`, data);
    return null;
  }

  get(data) {
    if (data instanceof ArrayBuffer) {
      const [key, ...array] = new Uint8Array(data);
      const name = this.findNameKey(key);
      if (name === 'INIT' && !this.isInit) {
        const object = this.parseBytesToJson(array);
        if (object) this.init(object);
        return;
      }
      const struct = this.structs[name];
      if (struct) {
        const object = struct.setBuffer(data).getObject();
        return { object, key: name };
      }
    }
    console.warn(`No struct from arr: ${data}`);
    return null;
  }
}

export default StructsToJSON;
