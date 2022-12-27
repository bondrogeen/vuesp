import Struct from 'c-struct-to-json';

class StructsToJSON {
  #keys;
  #structs;
  #isOk;

  constructor() {
    this.#keys = [];
    this.#structs = {};
    this.#isOk = false;
  }
  init({ keys = [], structs = {} }) {
    this.#keys = keys;
    for (const key in structs) {
      const name = key.toUpperCase();
      this.#structs[name] = new Struct(structs[key]);
      this.#isOk = true;
    }
  }

  set(key, data) {
    console.log(this.#isOk);
    if (!this.#isOk) {
      console.warn(`Struct not init`);
      return false;
    }
    const struct = this.#structs[key];
    const index = this.#keys.findIndex(i => i === key);
    if (struct && data) {
      return struct.setObject({ ...data, key: index }).getBuffer();
    }
    if (typeof index !== 'undefined') return this.#structs['INIT'].setObject({ key: index }).getBuffer();
    console.warn(`No struct or key ${key}`, data);
    return null;
  }

  get(data) {
    if (!this.#isOk) {
      console.warn(`Struct not init`);
      return false;
    }
    if (data instanceof ArrayBuffer) {
      const [key] = new Uint8Array(data);
      const name = this.#keys[key];
      const struct = this.#structs[name];
      if (struct) {
        const object = struct.setBuffer(data).getObject();
        return { object, key: name };
      }
      console.warn(`No struct in data`);
      return null;
    }
    console.warn(`No data or data not ArrayBuffer`);
    return null;
  }
}

export default StructsToJSON;
