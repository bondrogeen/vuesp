import { defineStore } from 'pinia';

export const useWebSocketStore = defineStore('websocketstore', {
  state: () => ({
    info: null,
    scanList: {},
    fileList: [],
    settings: {},
    unknown: null,
  }),
  actions: {
    SET_INFO(info) {
      this.info = info;
    },
    SET_SCAN({ id, name, clean }) {
      this.scanList = clean ? {} : { ...this.scanList, [id]: name };
    },
    SET_FILES(data) {
      this.fileList = data.clean ? [] : [...this.fileList, data];
    },
    SET_SETTINGS(value) {
      this.settings = value;
    },
    SET_UNKNOWN(value) {
      this.unknown = value;
    },
  },
  getters: {
    isConnect(state) {
      return state.pingClient - state.pingDevice < 2000;
    },
  },
});
