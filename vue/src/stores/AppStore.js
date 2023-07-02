import { defineStore } from 'pinia';

export const useAppStore = defineStore('app', {
  state: () => ({
    isLoading: false,
    theme: localStorage.getItem('theme') === 'dark',
    dialog: {
      value: false,
    },
    struct: {},
  }),
  actions: {
    init() {
      this.theme = localStorage.getItem('theme') === 'dark';
      document.documentElement.setAttribute('data-theme', this.theme ? 'dark' : 'light');
    },
    changeTheme(value) {
      this.theme = typeof value === 'undefined' ? !this.theme : Boolean(value);
      document.documentElement.setAttribute('data-theme', this.theme ? 'dark' : 'light');
      localStorage.setItem('theme', this.theme ? 'dark' : 'light');
    },
    setDialog(data) {
      this.dialog = data;
      // this.dialog.value = true;
    },
  },
});
