import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { localGet, localSet } from 'vuesp-components/helpers';

import type { IMessageNotification, IStoreWebSocketStore, IStateMain, IStateInfo, IMessageSettings, IMessagePort, IMessageProgress, TypeMessage } from '@/types';

const initialState = (): IStoreWebSocketStore => ({
  progress: { status: 0, empty: 0, size: 0, length: 0 },
  settings: { wifiDhcp: 1, wifiMode: 1, authMode: 1, version: 1, device: 0, wifiIp: [], wifiSubnet: [], wifiGateway: [], wifiDns: [], wifiSsid: '', wifiPass: '', authLogin: '', authPass: '' },
  main: {
    ports: {},
    info: { id: 0, firmware: [], totalBytes: 0, usedBytes: 0, uptime: 0, name: '' },
    device: { now: 0, pwm: 0, analog: 0 },
    dallas: {},
  },
  notifications: localGet('notifications') || [],
});

export const useWebSocketStore = defineStore('webSocketStore', {
  state: initialState,
  actions: {
    SET_INFO(info: IStateInfo) {
      this.main.info = info;
      this.main = { ...this.main };
    },
    SET_SETTINGS(value: IMessageSettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: IMessageProgress) {
      this.progress = value;
    },
    SET_PORT(port: IMessagePort) {
      const gpio = port.gpio.toString();
      this.main.ports[gpio] = port;
      this.main = { ...this.main };
    },
    SET_DALLAS(data: { address: number[] }) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
      this.main = { ...this.main };
    },
    SET_MAIN({ object, key }: { object: any; key: string }) {
      const name: keyof IStateMain = key.toLowerCase() as keyof IStateMain;
      if (['ping', 'files', 'progress', 'scan'].includes(name)) return;
      this.main[name] = object;
      this.main = { ...this.main };
    },
    SET_NOTIFICATION(notification: IMessageNotification) {
      const id = notification?.id || Date.now();
      this.notifications = [...this.notifications, { ...notification, id }];
      localSet('notifications', this.notifications);
    },
    onSend(key: TypeMessage['key'], object?: TypeMessage['object']) {
      const store = useWebSocket();
      store.onSend(key, object);
    },
  },
  getters: {
    isConnect: () => useWebSocket()?.isConnect || false,
  },
});
