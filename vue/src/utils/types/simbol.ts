import type { InjectionKey, Ref } from 'vue';

import type { IStorePackage, IDialog, INotificationItem } from 'vuesp-components/types';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
export const PKGKey: InjectionKey<IStorePackage> = Symbol('vuesp');
export const DialogKey: InjectionKey<(data: IDialog) => void> = Symbol('dialog');
export const NotificationKey: InjectionKey<(data: INotificationItem) => void> = Symbol('notification');
