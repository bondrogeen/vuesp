import type { InjectionKey, Ref } from 'vue';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
