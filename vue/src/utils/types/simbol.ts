import type { InjectionKey, Ref } from 'vue';

import type { IDialog, INotificationItem } from 'vuesp-components/types';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
export const VuespKey: InjectionKey<Ref<string>> = Symbol('vuesp');
export const DialogKey: InjectionKey<(data: IDialog) => void> = Symbol('dialog');
export const NotificationKey: InjectionKey<(data: INotificationItem) => void> = Symbol('notification');
