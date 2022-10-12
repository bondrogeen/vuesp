import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { useAppStore } from '@/stores/AppStore';

export const useWebSocketStore = defineStore('websocketstore', {
  state: () => ({
    info: {},
    progress: {},
    scanList: [],
    fileList: [],
    settings: {},
    unknown: null,
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
    SET_UNKNOWN(value) {
      this.unknown = value;
    },
    onSend(comm, data) {
      const store = useWebSocket();
      store.onSend(comm, data);
    },
  },
  getters: {
    isConnect() {
      const webSocket = useWebSocket();
      return webSocket.isConnect;
    },
  },
});
