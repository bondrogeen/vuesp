import { defineStore } from 'pinia';
import { changeTheme, localGet, isNewVersion, useFetch } from 'vuesp-components/helpers';
import { pathList } from '@/utils/const';

import type { IStoreApp, IDialog } from '@/types';

const initialState = (): IStoreApp => ({
  theme: 'dark',
  dialog: { value: false },
  menu: [],
  dashboard: [],
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
      const res = await useFetch.$get(`/default.json`, { method: 'GET' });
      this.menu = res?.menu || [];
      const dashboard = await useFetch.$get(`/fs?file=${pathList}`);
      this.dashboard = dashboard || res?.dashboard || [];
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
