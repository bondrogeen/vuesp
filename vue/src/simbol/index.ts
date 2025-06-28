import type { InjectionKey, Ref } from 'vue';

import type { TypeDialog, TypeNotificationItem } from '@/types/types.ts';

export const ThemeKey: InjectionKey<Ref<string>> = Symbol('theme');
export const DialogKey: InjectionKey<(data: TypeDialog) => void> = Symbol('dialog');
export const NotificationKey: InjectionKey<(data: TypeNotificationItem) => void> = Symbol('notification');
