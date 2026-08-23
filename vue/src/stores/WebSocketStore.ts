import { defineStore } from 'pinia';
import { useWebSocket } from '@/stores/WebSocket';
import { useAppStore } from './AppStore.ts';

import type { TypeMessage, IStateMain, IWebSocketStore, IMessagePort, IMessageDallas, IMessageProgress, IMessageMessage, IMessageDiscovery, IMyMessageSettings } from '@/types';

const initialState = (): IWebSocketStore => ({
  main: {
    ports: {},
    info: { id: 0, firmware: [], totalBytes: 0, usedBytes: 0, uptime: 0, name: '', board: 0 },
    device: {
      key: 9,
      command: 0,
      voltage: 0,
      current: 0,
      capacity: 0,
      fccRaw: 0,
      cycle: 0,
      empty: 0,
      dccRaw: 0,
      SOC: 0,
      protectionStatus: 0,
      cell1: 0,
      cell2: 0,
      cell3: 0,
      cell4: 0,
      temp1: 0,
      temp2: 0,
      temp3: 0,
      temp4: 0,
      cellStatus1: 0,
      cellStatus2: 0,
      cellStatus3: 0,
      cellStatus4: 0,
      tempStatus1: 0,
      tempStatus2: 0,
      tempStatus3: 0,
      tempStatus4: 0,
      chgCurrStatus: 0,
      totalVoltStatus: 0,
      dsgCurrStatus: 0,
      cellCount: 0,
      status1: 0,
      status2: 0,
      status3: 0,
      status4: 0,
      status5: 0,
      status6: 0,
      status7: 0,
      status8: 0,
      status9: 0,
      tempCount: 0,
      empty1: 0,
      empty2: 0,
      version: '',
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
    },
    SET_DALLAS(data: IMessageDallas) {
      const name = (data.address || []).map((i) => (i < 16 ? `0${i.toString(16)}` : i.toString(16))).join('');
      this.main.dallas[name] = data;
    },
    SET_DISCOVERY(data: IMessageDiscovery) {
      const id = data.id.toString(16);
      this.main.discovery[id] = data;
      // if(data.status) appStore.setNotification({text: `new${}`})
    },
    SET_MAIN({ object, key }: { object: any; key: string }) {
      const name: keyof IStateMain = key.toLowerCase() as keyof IStateMain;
      if (['ping', 'files', 'progress', 'scan'].includes(name)) return;
      this.main[name] = object;
    },
    SET_MESSAGE(message: IMessageMessage) {
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
    onSend(key: TypeMessage['key'], object?: TypeMessage['object']) {
      const store = useWebSocket();
      store.onSend(key, object);
    },
  },
});
