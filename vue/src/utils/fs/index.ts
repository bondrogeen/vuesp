import { pathConfigDef, pathConfig } from '@/utils/const';

export const getFile = (path: string) => fetch(`/fs?file=${path}`);

export const saveConfig = (data: any) => {
  return onUploadBinary(pathConfig, JSON.stringify(data));
};

const mixinObjects = (obj1: any = {}, obj2: any = {}) => {
  const obj: any = {};
  for (const key in obj1) {
    obj[key] = { ...obj2[key], ...obj1[key] };
  }
  for (const key in obj2) {
    if (!obj1[key]) {
      obj[key] = obj2[key];
    }
  }
  return obj;
};

export const getConfig = async (mixin = {}) => {
  const def = await fetchJSON(pathConfigDef);
  const res = await getFileJSON(pathConfig);
  const config = res || def;
  return mixinObjects(config, mixin);
};

export const fetchJSON = async (path: string) => {
  const def = await fetch(path);
  return await def.json();
};

export const getFileJSON = async (path: string) => {
  const res = await getFile(path);
  return res.ok ? await res.json() : null;
};

export const uploadFile = (body: any) => fetch('/fs', { method: 'POST', body });

export const getBinary = async (path: string) => {
  const { body, status }: any = await getFile(path);
  const res: any = await body.getReader().read();
  const data = Array.from(res.value);
  return status === 200 ? data : [];
};

export const onUploadBinary = async (path: string, buffer: any) => {
  const blob = new Blob([buffer], { type: 'octet/stream' });
  var data = new FormData();
  data.append(`file[0]`, blob, path);
  return uploadFile(data);
};
