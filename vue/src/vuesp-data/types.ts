export interface TypePropertyString {
  id: string;
  name: string;
  key: string;
  icon?: string;
  type?: string;
  minMax?: number[];
  get?: string;
  set?: string;
  modifyValue?: string;
}

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
