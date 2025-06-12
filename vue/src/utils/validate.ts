export const validateIP = (ip: string) =>
  /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ip);

export const minSet = (min: number, v: string) => v.length >= min;
export const maxSet = (max: number, v: string) => v.length <= max;

export const required = (value: string) => !!value || 'Required.';
export const sameAs = (value1: string, value: string) => value1 === value || 'Passwords do not match';
export const ip = (v: string) => validateIP(v) || 'Invalid ip address';
export const min = (i: number) => (v: string) => minSet(8, v) || `Min value ${i}`;
export const max = (i: number) => (v: string) => maxSet(32, v) || `Max value ${i}`;
export const isPort = (v: string) => (typeof +v === 'number' && !isNaN(+v) && +v < 65536) || 'Invalid port';
