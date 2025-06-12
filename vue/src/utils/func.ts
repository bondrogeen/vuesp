export const saveObjectWithFunctions = (obj: any) => {
  const processValue = (value: any) => {
    if (typeof value === 'function') return `${value.toString()}`;
    if (Array.isArray(value)) {
      const items: any = value.map((v) => processValue(v));
      return `[${items.join(',')}]`;
    }
    if (typeof value === 'object' && value !== null) {
      const entries: any = Object.entries(value).map(([key, val]) => `${JSON.stringify(key)}: ${processValue(val)}`);
      return `{${entries.join(',')}}`;
    }
    return JSON.stringify(value);
  };
  return processValue(obj);
};

export const safeEval = (code: any) => {
  if (typeof code !== 'string' || code.trim() === '') {
    return null;
  }
  try {
    return eval(code);
  } catch (error) {
    return null;
  }
};

export const stringToFunction = ({ set, get, ...all }: any) => {
  if (set && typeof set === 'string') {
    set = safeEval(set);
  }
  if (get && typeof get === 'string') {
    get = safeEval(get);
  }
  return { ...all, set, get };
};
