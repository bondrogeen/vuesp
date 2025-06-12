interface TypeProperty {
  name: string;
  key: string;
  get?: (obj: any) => string;
  set?: (obj: any) => string;
}

class Property {
  name: string;
  key: string;
  setProperty?: (obj: any, value: any) => string;
  getProperty?: (obj: any) => string;
  constructor({ name, key, get, set }: TypeProperty) {
    this.name = name;
    this.key = key;
    this.getProperty = get;
    this.setProperty = set;
  }

  getValue(name = '', obj = {}) {
    const keys = name.split('.');
    let current: any = obj;
    for (const key of keys) {
      if (current !== null && typeof current === 'object' && key in current) {
        current = current[key];
      } else {
        return undefined;
      }
    }
    return current;
  }

  get(obj: any) {
    const value = this.getValue(this.key, obj);
    if (this.getProperty) {
      return this.getProperty(value);
    }
    return value;
  }

  set(obj: any, value: any) {
    const byte = this.getValue(this.key, obj);
    if (this.setProperty) {
      return this.setProperty(byte, value);
    }
    return null;
  }
}

export class VuespData {
  list;
  data: any;
  constructor(list: any[]) {
    this.list = new Map();
    for (let i = 0; i < list.length; i++) {
      const item = list[i];
      this.list.set(item.id, new Property(item));
    }
  }

  setDate(data: any) {
    this.data = { ...this.data, ...data };
    return this;
  }

  get(name: string, object: any) {
    const data = this.list.get(name);
    return data ? data.get(object) : null;
  }

  set(name: string, object: any, value: any) {
    const data = this.list.get(name);
    return data ? data.set(object, value) : null;
  }

  getList() {
    const list = [];
    for (let [key] of this.list) {
      list.push(key);
    }
    return list;
  }
}
