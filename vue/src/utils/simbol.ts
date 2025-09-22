import type { InjectionKey, Ref } from 'vue';

import type { IPackage, IDialog, IMessageNotification } from 'vuesp-components/types';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
export const PKGKey: InjectionKey<IPackage> = Symbol('vuesp');
export const DialogKey: InjectionKey<(data: IDialog) => void> = Symbol('dialog');
export const NotificationKey: InjectionKey<(data: IMessageNotification) => void> = Symbol('notification');
