import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket.js';
import { useAppStore } from '@/stores/AppStore.js';

import type { IStateWebSocket, IStateMain, IStateInfo, IStateFile, IStateSettings, IStateScan, IGpio } from '@/types/types.ts';

const state: IStateWebSocket = {
  progress: {},
  scanList: [],
  fileList: [],
  path: ['root'],
  settings: {},
  gpio: {},
  unknown: undefined,
  main: {
    info: {},
    device: {},
    dallas: {},
  },
};

export const useWebSocketStore = defineStore('websocketstore', {
  state: () => state,
  actions: {
    SET_INFO(info: IStateInfo) {
      this.main.info = info;
      this.main = { ...this.main };
    },
    SET_SCAN(data: IStateScan) {
      this.scanList = [...this.scanList, data];
    },
    SET_FILES(data: IStateFile) {
      this.fileList = [...this.fileList, data];
    },
    SET_SETTINGS(value: IStateSettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: any) {
      const app = useAppStore();
      app.setNotification({ id: 1, text: 'Progress...', timeout: 60, ...value });
      this.progress = value;
    },
    SET_PORT(value: IGpio) {
      this.gpio[value.gpio] = value;
    },
    SET_DALLAS(data: { address: number[] }) {
      const name = (data.address || []).map((i) => (i < 15 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
      this.main = { ...this.main };
    },
    SET_MAIN({ object, key }: any) {
      const name: keyof IStateMain = key.toLowerCase();
      this.main[name] = object;
      this.main = { ...this.main };
    },
    onSend(comm: string, data?: any) {
      const store = useWebSocket();
      store.onSend(comm, data);
    },
  },
  getters: {
    isConnect: () => useWebSocket()?.isConnect || false,
  },
});
