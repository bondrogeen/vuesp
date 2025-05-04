export const jsonParse = (data: string | null) => {
  try {
    return data ? JSON.parse(data) : null;
  } catch (error) {
    return data;
  }
};

const jsonString = (data: object) => (typeof data === 'object' ? JSON.stringify(data) : data);

export const localGet = (name: string) => jsonParse(localStorage.getItem(name));
export const localSet = (name: string, data: any) => {
  if (localStorage) localStorage.setItem(name, jsonString(data));
};

export const sessionGet = (name: string) => jsonParse(sessionStorage.getItem(name));
export const sessionSet = (name: string, data: any) => {
  if (sessionStorage) sessionStorage.setItem(name, jsonString(data));
};

export const changeTheme = (value: string) => {
  localSet('theme', value);
  document.documentElement.classList.remove('dark');
  document.documentElement.classList.remove('light');
  document.documentElement.classList.add(value);
};
