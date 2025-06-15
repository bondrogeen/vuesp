export interface TypePropertyString {
  id: string;
  name: string;
  key: string;
  get?: string;
  set?: string;
  modifyValue?: string;
  type?: string;
  minMax?: number[];
}

export const saveObjectWithFunctions = (obj: any) => {
  const processValue = (value: any) => {
    if (typeof value === 'function') return `${value.toString()}`;
    if (Array.isArray(value)) {
      const items: any = value.map((v) => processValue(v));
      return `[${items.join(',')}]`;
    }
    if (typeof value === 'object' && value !== null) {
      const entries: any = Object.entries(value).map(([key, val]) => `${JSON.stringify(key)}: ${processValue(val)}`);
      return `{${entries.join(',')}}`;
    }
    return JSON.stringify(value);
  };
  return processValue(obj);
};

export const safeEval = (code: any) => {
  if (typeof code !== 'string' || code.trim() === '') {
    return null;
  }
  try {
    return eval(code);
  } catch (error) {
    return null;
  }
};

export const stringToFunction = ({ set, get, ...all }: any) => {
  if (set && typeof set === 'string') {
    set = safeEval(set);
  }
  if (get && typeof get === 'string') {
    get = safeEval(get);
  }
  return { ...all, set, get };
};

export const functionToString = ({ set, get, modifyValue, ...arg }: TypeProperty) => {
  console.log(arg);

  const obj: TypePropertyString = { ...arg };
  if (set) obj.set = set.toString();
  if (get) obj.get = get.toString();
  if (modifyValue) obj.get = modifyValue.toString();
  return obj;
};

export const createObjectFromPaths = (paths: string[], delimiter = '.') => {
  const result = {};

  for (const path of paths) {
    const keys = path.split(delimiter);
    let current: any = result;

    for (let i = 0; i < keys.length; i++) {
      const key = keys[i];
      if (!current[key]) {
        current[key] = i === keys.length - 1 ? path : {};
      } else if (current[key] === true && i < keys.length - 1) {
        current[key] = {};
      }
      if (i < keys.length - 1) {
        current = current[key];
      }
    }
  }

  return result;
};

export interface TypeProperty {
  id: string;
  name: string;
  key: string;
  type?: string;
  icon?: string;
  value?: any;
  minMax?: number[];
  get?: (output: any) => any;
  set?: (output: any, value: any) => any;
  modifyValue?: (value: any) => any;
  getItem?: (output: any) => any;
  setItem?: (output: any, value: any) => any;
}

class Property implements TypeProperty {
  id: string;
  name: string;
  key: string;
  type?: string;
  icon?: string;
  minMax?: number[];
  set?: (obj: any, value: any) => any;
  get?: (obj: any) => any;
  modifyValue?: (obj: any) => any;

  constructor({ id, name, key, type, icon, minMax, get, set, modifyValue }: TypeProperty) {
    this.id = id;
    this.name = name;
    this.key = key;
    this.type = type;
    this.icon = icon;
    this.minMax = minMax;
    this.set = set;
    this.get = get;
    this.modifyValue = modifyValue;
  }

  getItem(value: any) {
    if (this.get) {
      return this.get(value);
    }
    return null;
  }

  setItem(data: any, value: any) {
    if (this.set) {
      return this.set(data, value);
    }
    return null;
  }
}

export interface TypeVuespData {
  listDef: TypeProperty[];
  items: Map<string, TypeProperty>;
  data: any;
  removeItem: (id: string) => void;
  editItem: (id: string, item: TypePropertyString) => void;
  setData: (data: any) => TypeVuespData;
  getData: () => any;
  saveList: () => string;
  getList: () => TypeProperty[];

  get: (id: string) => any;
  set: (id: string, value: any) => any;
}

export class VuespData implements TypeVuespData {
  listDef: TypeProperty[];
  items: Map<string, TypeProperty>;
  data: any;

  constructor(list: TypeProperty[]) {
    this.listDef = list;
    this.items = new Map();
    this.init();
  }

  init() {
    for (let i = 0; i < this.listDef.length; i++) {
      const item: TypeProperty = this.listDef[i];
      this.items.set(item.id, new Property(item));
    }
  }

  setDataValue({ key }: TypeProperty, value: any) {
    const keys = key.split('.');
    let current = this.data;
    for (let i = 0; i < keys.length - 1; i++) {
      current = current[keys[i]];
    }
    current[keys[keys.length - 1]] = value;
  }

  getDataValue({ key }: TypeProperty) {
    const keys = key.split('.');

    let current: any = this.data;
    for (const key of keys) {
      if (current !== null && typeof current === 'object' && key in current) current = current[key];
      else return undefined;
    }
    return current;
  }

  setData(data: any) {
    this.data = { ...data };
    return this;
  }

  getData() {
    return this.data;
  }

  get(id: string) {
    const item = this.items.get(id);
    if (!item) return false;
    const data = this.getDataValue(item);
    return item?.getItem ? item.getItem(data) : null;
  }

  set(id: string, value: any) {
    const item = this.items.get(id);
    if (!item) return false;
    const data = this.getDataValue(item);
    const update = item?.setItem ? item.setItem(data, value) : null;
    if (update !== null) {
      this.setDataValue(item, update);
    }
    return update;
  }

  editItem(id: string, item: TypePropertyString) {
    if (item) {
      this.items.set(id, stringToFunction(item));
    }
  }

  removeItem(id: string) {
    this.items.delete(id);
  }

  saveList() {
    const list = Array.from(this.items, ([_, item]) => ({ ...item }));
    let content = saveObjectWithFunctions(list);
    return `export default ${content};`;
  }

  getList() {
    return Array.from(this.items, ([_, item]) => item);
  }

  getObjectList() {
    const paths = Array.from(this.items, ([_, item]) => item.id);
    return createObjectFromPaths(paths);
  }

  getPaths() {
    const result: string[] = [];

    const traverse = (currentObj: any, currentPath: string) => {
      for (const key in currentObj) {
        if (Object.hasOwnProperty.call(currentObj, key)) {
          const value = currentObj[key];
          const newPath = currentPath ? `${currentPath}.${key}` : key;

          if (typeof value === 'object' && value !== null) {
            traverse(value, newPath);
          } else {
            result.push(newPath);
          }
        }
      }
    };

    traverse(this.data, '');
    return result;
  }
}
