import { pathConfigDef, pathConfig } from '@/utils/const';

export const getFile = path => fetch(`/fs?file=${path}`);

export const saveConfig = (data) => {
  return onUploadBinary(pathConfig, JSON.stringify(data));
}

const mixinObjects = (obj1, obj2) => {
  var c = {}
  for (const key in obj1) {
    if (obj1[key] && obj2[key]) {
      c[key] = obj2[key] || obj1[key]
    } else{
      
    }
  }
  return c;
}

export const getConfig = async (mixin = {}) => {
  const def = await fetchJSON(pathConfigDef);
  const res = await getFileJSON(pathConfig);
  const config = res || def
  return mixinObjects(config, mixin);
}

export const fetchJSON = async (path) => {
  const def = await fetch(path);
  return await def.json()
}

export const getFileJSON = async (path) => {
  const res = await getFile(path)
  return res.ok ? await res.json() : null
}

export const uploadFile = body => fetch('/fs', { method: 'POST', body });

export const getBinary = async path => {
  const { body, status } = await getFile(path);
  return status === 200 ? Array.from((await body.getReader().read()).value) : [];
};

export const onUploadBinary = async (path, buffer) => {
  const blob = new Blob([buffer], { type: 'octet/stream' });
  var data = new FormData();
  data.append(`file[0]`, blob, path);
  return uploadFile(data);
};
