import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { KEY } from '@/generated/protocol';
import type { TKey } from '@/generated/protocol';
import { useAppStore } from './AppStore.ts';

import type { TMessage } from '@/types';
import type { IWebSocketStore, IPort, IDallas, IProgress, IMessage, IDiscovery, IInfo, IDevice, IBuffer, IEvent, ISettings } from '@/types';

const initialState = (): IWebSocketStore => ({
  main: {
    ports: {},
    info: { id: 0, firmware: [], totalBytes: 0, usedBytes: 0, uptime: 0, name: '', board: 0 },
    device: { command: 0, analog: 0, now: 0, list: 0, message: '' },
    buffer: { empty: 0, head: 0, tail: 0, count: 0, data: [] },
    event: { command: 0, subCommand: 0, value1: 0, value2: 0, name: '' },
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
  message: { type: 0, active: 0, handler: 0, date: 0, index: 0, id: 0, len: 0, text: '' },
});

export const useWebSocketStore = defineStore('webSocketStore', {
  state: initialState,
  actions: {
    SET_SETTINGS(value: ISettings) {
      this.settings = value;
    },
    SET_PROGRESS(value: IProgress) {
      this.progress = value;
    },
    SET_PORT(port: IPort) {
      const gpio = port.gpio.toString();
      this.main.ports[gpio] = port;
    },
    SET_DALLAS(data: IDallas) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
    },
    SET_DISCOVERY(data: IDiscovery) {
      const id = data.id.toString(16);
      this.main.discovery[id] = data;
    },
    SET_INFO(value: IInfo) {
      this.main.info = value;
    },
    SET_DEVICE(value: IDevice) {
      this.main.device = value;
    },
    SET_BUFFER(value: IBuffer) {
      this.main.buffer = value;
    },
    SET_EVENT(value: IEvent) {
      this.main.event = value;
    },
    SET_MESSAGE(message: IMessage) {
      this.message = message;
      const { type } = message;
      if (type === 1) {
        const appStore = useAppStore();
        appStore.setNotification(message)
      }
      if (type === 2) {
        const index = message.index;
        this.main.slots[index || 0] = message;
      }
    },
    onMessage({ key, object }: TMessage) {
      switch (key) {
        case KEY.SETTINGS:
          this.SET_SETTINGS(object);
          return;
        case KEY.PROGRESS:
          this.SET_PROGRESS(object);
          return;
        case KEY.PORT:
          this.SET_PORT(object);
          return;
        case KEY.DALLAS:
          this.SET_DALLAS(object);
          return;
        case KEY.DISCOVERY:
          this.SET_DISCOVERY(object);
          return;
        case KEY.MESSAGE:
          this.SET_MESSAGE(object);
          return;
        case KEY.INFO:
          this.SET_INFO(object);
          return;
        case KEY.DEVICE:
          this.SET_DEVICE(object);
          return;
        case KEY.BUFFER:
          this.SET_BUFFER(object);
          return;
        case KEY.EVENT:
          this.SET_EVENT(object);
          return;
        default:
          return;
      }
    },
    onSend(key: TKey | number, object?: object) {
      const store = useWebSocket();
      store.onSend(key, object);
    },
  },
});
