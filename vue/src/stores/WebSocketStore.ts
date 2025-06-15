import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket.js';
import { useAppStore } from '@/stores/AppStore.js';

import type { TypeStateWebSocket, TypeStateInfo, TypeStateFile, TypeStateSettings, TypeStateScan, TypeGpio } from '@/types/types.ts';

const state: TypeStateWebSocket = {
  progress: {},
  scanList: [],
  fileList: [],
  path: ['root'],
  settings: {},
  gpio: {},
  unknown: {},
  main: {
    info: {},
    device: {},
    dallas: {},
  },
  // modbus: {},
};

export const useWebSocketStore = defineStore('websocketstore', {
  state: () => state,
  actions: {
    SET_INFO(info: TypeStateInfo) {
      this.main.info = info;
      this.main = { ...this.main };
    },
    SET_SCAN(data: TypeStateScan) {
      this.scanList = [...this.scanList, data];
    },
    SET_FILES(data: TypeStateFile) {
      this.fileList = [...this.fileList, data];
    },
    SET_SETTINGS(value: TypeStateSettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: any) {
      const app = useAppStore();
      app.setNotification({ id: 1, text: 'Progress...', timeout: 60, ...value });
      this.progress = value;
    },
    SET_PORT(value: TypeGpio) {
      this.gpio[value.gpio] = value;
    },
    SET_DEVICE(value: any) {
      this.main.device = value;
      this.main = { ...this.main };
    },
    SET_DALLAS(data: { address: number[] }) {
      const name = (data.address || []).map((i) => (i < 15 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
      this.main = { ...this.main };
    },
    // SET_MODBUS(value: any) {
    //   this.modbus = value;
    // },
    SET_UNKNOWN({ object, key }: any) {
      this.unknown[key] = object;
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
