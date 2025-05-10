import type { TypeMenu } from '@/types/types.ts';

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

export const findMenuTitle = (items: any, path: string) => items.reduce((acc: any, i: any) => (i.path === path ? i : i.children ? findMenuTitle(i.children, path) : acc), null);

export const getPageTitle = (menu: TypeMenu[], path: string) => menu.reduce((acc: any, i: TypeMenu) => findMenuTitle(i.items, path) || acc, {});
