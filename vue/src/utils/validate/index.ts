export const validateIP = (ip: string) =>
  /^(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)\.(25[0-5]|2[0-4][0-9]|[01]?[0-9][0-9]?)$/.test(ip);

export const min = (min: number, v: string) => v.length >= min;
export const max = (max: number, v: string) => v.length <= max;

export const rules = {
  required: (value: string) => !!value || 'Required.',
  sameAs: (value1: string, value: string) => value1 === value || 'Passwords do not match',
  ip: (v: string) => validateIP(v) || 'Invalid ip address',
  min: (i: number) => (v: string) => min(8, v) || `Min value ${i}`,
  max: (i: number) => (v: string) => max(32, v) || `Max value ${i}`,
  isPort: (v: string) => (typeof +v === 'number' && !isNaN(+v) && +v < 65536) || 'Invalid port',
};
