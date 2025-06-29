import type { TypeMenuItem } from '@/types/types.ts';

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

export const getPageTitle = (menu: TypeMenuItem[], path: string) => findMenuTitle(menu, path);

export const toByte = (value: number) => {
  const sizes = ['B', 'KB', 'MB'];
  if (value == 0) return '0 B';
  let i = Math.floor(Math.log(value) / Math.log(1024));
  if (i > 1) i = 1;
  return Math.round(value / Math.pow(1024, i)) + ' ' + sizes[i];
};

export const dateUtcToString = (unixTime: number, options: Intl.DateTimeFormatOptions = { year: 'numeric', month: 'numeric', day: 'numeric' }, location = []) => {
  const localDate = new Date(unixTime);
  return localDate.toLocaleDateString(location, options);
};

export const timeUtcToString = (unixTime: number, options: Intl.DateTimeFormatOptions = { second: '2-digit', minute: '2-digit', hour: '2-digit', timeZone: 'UTC' }, location = []) => {
  const localDate = new Date(unixTime);
  return localDate.toLocaleTimeString(location, options);
};

const addZero = (value: number) => (value >= 10 ? value : `0${value}`);

export const secToTime = (seconds: number) => {
  // let y = Math.floor(seconds / 31536000);
  // let mo = Math.floor((seconds % 31536000) / 2628000);
  let d = Math.floor(((seconds % 31536000) % 2628000) / 86400);
  let h = Math.floor((seconds % (3600 * 24)) / 3600);
  let m = Math.floor((seconds % 3600) / 60);
  let s = Math.floor(seconds % 60);
  return `${d ? d + ' days ' : ''} ${addZero(h)}:${addZero(m)}:${addZero(s)}`;
};

export const debounce = (fn: Function, ms = 300) => {
  let timeoutId: ReturnType<typeof setTimeout>;
  return function (this: any, ...args: any[]) {
    clearTimeout(timeoutId);
    timeoutId = setTimeout(() => fn.apply(this, args), ms);
  };
};

export const hexToRGBA = (hex: any, a = 1) => [...hex.match(/\w\w/g).map((x: string) => parseInt(x, 16)), a];

export const int32ToBytes = (int: number) => [(int >> 24) & 0xff, (int >> 16) & 0xff, (int >> 8) & 0xff, int & 0xff];

export const objCopy = (obj: any) => {
  try {
    return JSON.parse(JSON.stringify(obj));
  } catch (error) {
    return null;
  }
};
