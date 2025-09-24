import type { InjectionKey, Ref } from 'vue';

import type { IPackage } from 'vuesp-components/types';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
export const PKGKey: InjectionKey<IPackage> = Symbol('vuesp');
