import { defineStore } from 'pinia';
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import VuespStruct from 'vuesp-struct';

import { useWebSocketStore } from './WebSocketStore.ts';

import event from '@/assets/js/event.ts';
import { log } from '@/utils/debug.ts';

const struct = new VuespStruct();

interface TypeWebSocket {
  socket: any;
  pingClient: number;
  pingDevice: number;
  struct: any;
}

const state: TypeWebSocket = {
  socket: null,
  pingClient: 5000,
  pingDevice: 0,
  struct: null,
};

export const useWebSocket = defineStore('websocket', {
  state: () => state,
  actions: {
    init() {
      event.emit('init');
      this.onSend('INFO');
      this.onSend('DEVICE');
      this.onSend('PORT');
      this.onSend('SENSORS');
    },
    async onStruct() {
      const res = await (await fetch(`/struct.json`, { method: 'GET' })).json();
      // console.log(res);
      struct.init(res);
      return res;
    },
    onopen() {
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      event.emit('connected', true);
      this.init();
    },
    onmessage(message: any) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const data = struct.get(message.data);
        if (data) {
          const { object, key } = data;
          // console.log(object);
          if (key !== 'PING') log(object, key);
          const store = useWebSocketStore();
          // eslint-disable-next-line @typescript-eslint/ban-ts-comment
          // @ts-ignore
          if (store?.[`SET_${key}`]) store?.[`SET_${key}`](object);
          else store.SET_UNKNOWN(data);
        }
      }
    },
    onclose(data: string) {
      event.emit('connected', false);
      log(data);
    },
    onerror(data: string) {
      event.emit('connected', false);
      log(data);
    },
    onSend(comm: string, data?: any) {
      log(comm, data);
      if (this?.socket?.readyState && this?.socket?.send && this.isConnect) {
        const buffer = struct.set(comm, data);
        if (buffer) this.socket.send(buffer);
      }
    },
    onPing() {
      this.pingClient = Date.now();
    },
  },
  getters: {
    isConnect: (state) => state.pingClient - state.pingDevice < 10000,
  },
});
