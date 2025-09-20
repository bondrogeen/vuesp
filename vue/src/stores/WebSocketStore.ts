import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket.js';
import { useAppStore } from '@/stores/AppStore.js';

import type { IStoreWebSocket, IStoreMain, IStoreInfo, IStoreFile, IStoreSettings, IStoreScan, IStoreGpio } from 'vuesp-components/types';

const initialState = (): IStoreWebSocket => ({
  progress: {},
  scanList: [],
  fileList: [],
  path: ['root'],
  settings: {},
  main: {
    gpio: {},
    info: {},
    device: {},
    dallas: {},
  },
});

export const useWebSocketStore = defineStore('webSocketStore', {
  state: initialState,
  actions: {
    SET_INFO(info: IStoreInfo) {
      this.main.info = info;
      this.main = { ...this.main };
    },
    SET_SCAN(data: IStoreScan) {
      this.scanList = [...this.scanList, data];
    },
    SET_FILES(data: IStoreFile) {
      this.fileList = [...this.fileList, data];
    },
    SET_SETTINGS(value: IStoreSettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: any) {
      const app = useAppStore();
      app.setNotification({ id: 1, text: 'Progress...', timeout: 60, ...value });
      this.progress = value;
    },
    SET_PORT(value: IStoreGpio) {
      this.main.gpio[value.gpio] = value;
      this.main = { ...this.main };
    },
    SET_DALLAS(data: { address: number[] }) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
      this.main = { ...this.main };
    },
    SET_MAIN({ object, key }: { object: any; key: string }) {
      const name: keyof IStoreMain = key.toLowerCase() as keyof IStoreMain;
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
