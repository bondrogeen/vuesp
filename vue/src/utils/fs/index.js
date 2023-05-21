export const getFile = path => fetch(`/fs?file=${path}`);

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
