export const validateIP = (ip: string) =>
  /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ip);

const minLength = (min: number, v: string) => v && v.length >= min;
const maxLength = (max: number, v: string) => v && v.length <= max;
const minValue = (min: number, v: string) => v && +v >= min;
const maxValue = (max: number, v: string) => v && +v <= max;

export const isDate = (v: unknown) => (v && v instanceof Date && !isNaN(v.valueOf())) || 'Invalid Date';
export const isNumber = (v: string) => (typeof v === 'number' && !isNaN(v)) || 'Value is a number';
export const required = (v: string) => Boolean(v && v.length) || 'Required';
export const sameAs =
  (name: string, m = 'Field do not match') =>
  (v: string, vals: Record<string, any> = {}) =>
    v === vals[name] || m;

export const isEmail = (v: string) => /^[^\s@]+@[^\s@]+\.[^\s@]+$/.test(v) || 'Invalid Email';
export const minLen = (i: number) => (v: string) => minLength(i, v) || `Min length ${i}`;
export const maxLen = (i: number) => (v: string) => maxLength(i, v) || `Max length ${i}`;
export const minVal = (i: number) => (v: string) => minValue(i, v) || `Min value ${i}`;
export const maxVal = (i: number) => (v: string) => maxValue(i, v) || `Max value ${i}`;

export const ip = (v: string) => validateIP(v) || 'Invalid ip address';
export const isPort = (v: string) => (typeof +v === 'number' && !isNaN(+v) && +v < 65536) || 'Invalid port';
