import en from '@/locale/en';

export const $t = (name: string) => {
  en?.[name] || name;
};
