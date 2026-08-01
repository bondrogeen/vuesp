import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { localGet, localSet } from 'vuesp-components/helpers';

import type { INotification, IStoreWebSocketStore, IStateMain, IMessagePort, IMessageProgress, TypeMessage, IMessageMessage, IDiscovery, IMyMessageSettings } from '@/types';

const initialState = (): IStoreWebSocketStore => ({
  main: {
    ports: {},
    info: { id: 0, firmware: [], totalBytes: 0, usedBytes: 0, uptime: 0, name: '', board: 0 },
    device: {
      key: 9,
      command: 0,
      voltage: 0,
      current: 0,
      balanceCapacity: 0,
      rateCapacity: 0,
      cycle: 0,
      empty: 0,
      balanceStatus: 0,
      balanceStatusHigh: 0,
      protectionStatus: 0,
      version: 0,
      rsoc: 0,
      fet: 0,
      series: 0,
      temp: 0,
      ntc: 0,
      ntc1: 0,
      ntc2: 0,
      ntc3: 0,
      cellLow: 0,
      cellHigh: 0,
      cellDiff: 0,
      cellAvg: 0,
      dateDay: 0,
      dateMonth: 0,
      dateYear: 0,
      cellVoltage: [],
    },
    dallas: {},
    discovery: {},
    slots: {},
  },
  settings: {
    wifiDhcp: 1,
    wifiMode: 1,
    authMode: 1,
    version: 1,
    device: 0,
    wifiIp: [],
    wifiSubnet: [],
    wifiGateway: [],
    wifiDns: [],
    wifiSsid: '',
    wifiPass: '',
    authLogin: '',
    authPass: '',
    discovery: 0,
    discoveryInterval: 5,
    discoveryPort: 0,
  },
  progress: { status: 0, empty: 0, size: 0, length: 0 },
  notifications: localGet('notifications', true) || [],
  message: { type: 0, active: 0, handler: 0, date: 0, index: 0, id: 0, len: 0, text: '' },
});

export const useWebSocketStore = defineStore('webSocketStore', {
  state: initialState,
  actions: {
    SET_SETTINGS(value: IMyMessageSettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: IMessageProgress) {
      this.progress = value;
    },
    SET_PORT(port: IMessagePort) {
      const gpio = port.gpio.toString();
      this.main.ports[gpio] = port;
      // this.main = { ...this.main };
    },
    SET_DALLAS(data: { address: number[] }) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
      // this.main = { ...this.main };
    },
    SET_DISCOVERY(data: IDiscovery) {
      const id = data.id.toString(16);
      this.main.discovery[id] = data;
      // this.main = { ...this.main };
    },
    SET_MAIN({ object, key }: { object: any; key: string }) {
      const name: keyof IStateMain = key.toLowerCase() as keyof IStateMain;
      if (['ping', 'files', 'progress', 'scan'].includes(name)) return;
      this.main[name] = object;
      // this.main = { ...this.main };
    },
    SET_MESSAGE(message: IMessageMessage) {
      this.message = message;
      const { type } = message;
      // if (type === 1) {
      //   const date = message?.date || Date.now();
      //   this.notifications = [...this.notifications, { key, color: 1, isNew, timeout, text, date }];
      //   localSet('notifications', this.notifications);
      // }
      if (type === 2) {
        const index = message.index;
        this.main.slots[index || 0] = message;
        // this.main = { ...this.main };
      }
    },
    READ_NOTIFICATION(notification: INotification) {
      this.notifications = this.notifications.map((i) => (notification.date === i.date ? { ...i, isNew: 0 } : i));
      localSet('notifications', this.notifications);
    },
    READ_ALL_NOTIFICATION() {
      this.notifications = this.notifications.map((i) => ({ ...i, isNew: 0 }));
      localSet('notifications', this.notifications);
    },
    REMOVE_NOTIFICATION(notification: INotification) {
      this.notifications = this.notifications.filter((i) => i.date !== notification.date);
      localSet('notifications', this.notifications);
    },
    onSend(key: TypeMessage['key'], object?: TypeMessage['object']) {
      const store = useWebSocket();
      store.onSend(key, object);
    },
  },
  getters: {},
});
