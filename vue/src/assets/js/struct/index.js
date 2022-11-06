import Struct from 'c-struct-to-json';

class StructToJSON {
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

  getCommand(key) {
    return typeof key === 'string' ? this.keys.findIndex(i => i === key) : this.keys[key];
  }

  set(key, data) {
    console.log(key, data);
    if (key) {
      const command = this.getCommand(key);
      if (data) {
        const struct = this.structs[key];
        if (struct) {
          return struct.setObject(data).getBuffer();
        }
      } else {
        return this.structs['INIT'].setObject({ key: command }).getBuffer();
      }
    }
    console.warn(`No struct or key ${key}`, data);
    return null;
  }

  get(data) {
    if (data instanceof ArrayBuffer) {
      const [key, ...array] = new Uint8Array(data);
      const comm = this.getCommand(key);
      if (comm === 'INIT' && !this.isInit) {
        const object = this.parseBytesToJson(array);
        if (object) this.init(object);
        return;
      }
      const struct = this.structs[comm];
      if (struct) {
        const object = struct.setBuffer(data).getObject();
        object.key = comm
        return object
      }
    }
    console.warn(`No struct from arr: ${data}`);
    return null;
  }
}

export default StructToJSON;
