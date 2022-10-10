import { defineStore } from 'pinia';

export const useAppStore = defineStore('app', {
  state: () => ({
    isLoading: false,
    dialog: {
      value: false,
    },
  }),
  actions: {
    setDialog(data) {
      this.dialog = data;
      this.dialog.value = true;
    },
  },
});
