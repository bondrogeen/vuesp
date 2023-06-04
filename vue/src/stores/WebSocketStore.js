import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { useAppStore } from '@/stores/AppStore';

export const useWebSocketStore = defineStore('websocketstore', {
  state: () => ({
    info: {},
    progress: {},
    scanList: [],
    fileList: [],
    path: ['root'],
    settings: {},
    gpios: {},
    unknown: {},
    device: {},
  }),
  actions: {
    SET_INFO(info) {
      this.info = info;
    },
    SET_SCAN(data) {
      this.scanList = [...this.scanList, data];
    },
    SET_FILES(data) {
      this.fileList = [...this.fileList, data];
    },
    SET_SETTINGS(value) {
      this.settings = value;
    },
    SET_PROGRESS(value) {
      if (value.status === 1) {
        const app = useAppStore();
        app.setDialog({ title: 'Progress', value: true, isProgress: true });
      }
      this.progress = value;
    },
    SET_PORT(value) {
      this.gpios[value.gpio] = value;
    },
    SET_DEVICE(value) {
      this.device = value;
    },
    SET_UNKNOWN({ object, key }) {
      this.unknown[key] = object;
    },
    onSend(comm, data) {
      const store = useWebSocket();
      store.onSend(comm, data);
    },
  },
  getters: {
    isConnect: () => useWebSocket()?.isConnect || false,
  },
});
