import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';

export const useWebSocketStore = defineStore('websocketstore', {
  state: () => ({
    info: {},
    progress: {},
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
      this.fileList = [...this.fileList, data];
    },
    SET_SETTINGS(value) {
      this.settings = value;
    },
    SET_PROGRESS(value) {
      this.progress = value;
    },
    SET_UNKNOWN(value) {
      this.unknown = value;
    },
    onSend(comm, data) {
      const store = useWebSocket();
      store.onSend(comm, data);
    },
  }
});
