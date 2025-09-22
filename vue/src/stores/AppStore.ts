import { defineStore } from 'pinia';
import { changeTheme, localGet, isNewVersion } from 'vuesp-components/helpers';

import type { IStoreApp, IDialog } from '@/types';

const initialState = (): IStoreApp => ({
  theme: 'dark',
  dialog: { value: false },
  menu: [],
  dialogInfo: isNewVersion(),
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
  },
});
