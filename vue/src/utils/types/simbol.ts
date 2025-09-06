import type { InjectionKey, Ref } from 'vue';

import type { IDialog, INotificationItem } from '@/utils/types/types.ts';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
export const DialogKey: InjectionKey<(data: IDialog) => void> = Symbol('dialog');
export const NotificationKey: InjectionKey<(data: INotificationItem) => void> = Symbol('notification');
