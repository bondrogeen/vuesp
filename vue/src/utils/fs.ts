import { pathListDef, pathList } from '@/utils/const.ts';
import { saveObjectWithFunctions } from '@/utils/func.ts';

export const getFile = (path: string) => fetch(`/fs?file=${path}`);

export const fetchJson = async (path: string, init?: RequestInit) => {
  const res = await fetch(path, init);
  return await res.json();
};
export const fetchText = async (path: string, init?: RequestInit) => {
  const res = await fetch(path, init);
  console.log(res);
  return await res.text();
};

export const loadModule = async (path: string, init?: RequestInit) => {
  const text = await fetchText(path, init);
  console.log(text);
  
  return await import('data:text/javascript,' + text);
};

export const saveModule = async (path: string, obj: any) => {
  let content = saveObjectWithFunctions(obj);
  content = `export const list = ${content};`;
  return onUploadBinary(path, content);
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
