import { defineStore } from 'pinia';
import { changeTheme, localGet, isNewVersion } from 'vuesp-components/helpers';
import { useFetch } from '@vueuse/core';
import { pathList } from '@/utils/const';

import type { IStoreApp, IDialog } from '@/types';

const initialState = (): IStoreApp => ({
  theme: localGet('theme') || 'dark',
  dialog: { value: false },
  menu: [],
  dashboard: [],
  dialogInfo: isNewVersion(),
  pkg: { name: '', version: '' },
});

export const useAppStore = defineStore('app', {
  state: initialState,
  actions: {
    async init({ theme }: { theme?: string }) {
      if (theme && ['light', 'dark'].includes(theme)) this.theme = theme;
      changeTheme(this.theme);

      const { data } = await useFetch(`/fs?file=${pathList}`).json();
      if (data.value) this.dashboard = data.value;
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
