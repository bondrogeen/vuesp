import { defineStore } from 'pinia';
import { changeTheme, localGet } from '@/utils/helpers';

import type { INotificationItem, IStateApp, IDialog } from 'vuesp-components/types';

const initialState = (): IStateApp => ({
  menu: [],
  isLoading: false,
  theme: 'dark',
  dialog: {
    value: false,
  },
  notifications: [],
  struct: {},
});

export const useAppStore = defineStore('app', {
  state: initialState,
  actions: {
    async init({ theme }: { theme?: string }) {
      this.theme = localGet('theme') || 'dark';
      if (theme && ['light', 'dark'].includes(theme)) {
        this.theme = theme;
      }
      changeTheme(this.theme);
      this.menu = await (await fetch(`/menu.json`, { method: 'GET' })).json();
    },
    changeTheme(value?: string) {
      this.theme = value || (this.theme === 'light' ? 'dark' : 'light');
      changeTheme(this.theme);
    },
    setDialog(data: IDialog) {
      this.dialog = data;
    },
    setNotification(notification: INotificationItem) {
      const id = notification?.id || Date.now();
      const timeout = notification?.timeout || 10;
      const idx = this.notifications.findIndex((i) => i.id === id);

      if (idx !== -1) {
        this.notifications[idx] = { ...this.notifications[idx], ...notification, id, timeout };
      } else {
        this.notifications = [...this.notifications, { ...notification, id, timeout }];
      }
    },
  },
});
