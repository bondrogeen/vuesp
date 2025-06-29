import { defineStore } from 'pinia';
import { changeTheme, localGet } from '@/utils/helpers.ts';

import type { TypeNotificationItem, TypeStateApp, TypeDialog } from '@/types/types.ts';

const state: TypeStateApp = {
  menu: [],
  isLoading: false,
  theme: 'dark',
  dialog: {
    value: false,
  },
  notifications: [],
  struct: {},
};

export const useAppStore = defineStore('app', {
  state: () => state,
  actions: {
    async init({ theme }: any) {
      this.theme = localGet('theme') || 'dark';
      if (['light', 'dark'].includes(theme)) {
        this.theme = theme;
      }
      changeTheme(this.theme);
      this.menu = await (await fetch(`/menu.json`, { method: 'GET' })).json();
    },
    changeTheme(value?: string) {
      this.theme = this.theme === 'light' ? 'dark' : 'light';
      changeTheme(value || this.theme);
    },
    setDialog(data: TypeDialog) {
      this.dialog = data;
    },
    setNotification(notification: TypeNotificationItem) {
      const id = notification?.id || Date.now();
      const timeout = notification?.timeout || 10;
      const item = this.notifications.find((i) => i.id === id);

      if (item) {
        this.notifications = this.notifications.map((i) => (i.id === item.id ? notification : i));
      } else {
        this.notifications = [...this.notifications, { ...notification, id, timeout }];
      }
    },
  },
});
