export const hexToRGBA = (hex: string, a = 1): number[] => [...hex.match(/\w\w/g)!.map((x) => parseInt(x, 16)), a];

export const int32ToBytes = (int: number): number[] => [(int >> 24) & 0xff, (int >> 16) & 0xff, (int >> 8) & 0xff, int & 0xff];

export const objCopy = <T>(obj: T): T | null => {
  try {
    return JSON.parse(JSON.stringify(obj)) as T;
  } catch {
    return null;
  }
};
