import { defineStore } from 'pinia';

export const useAppStore = defineStore('app', {
  state: () => ({
    isLoading: false,
    dialog: false,
    content: {
      message: '',
    },
  }),
  actions: {
    setDialog(data) {
      this.dialog = true;
      this.content = data;
      this.content.message = data?.message || '';
    },
  },
});
