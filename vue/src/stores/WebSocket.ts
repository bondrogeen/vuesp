import type { IStoreWebSocket } from '@/types';

import { defineStore } from 'pinia';
import { useWebSocketStore } from './WebSocketStore.ts';

import { struct, event } from '@/assets/js/';

const log = (...arg: any) => (process.env.NODE_ENV === 'development' ? console.log(arg) : null);

const initialState = (): IStoreWebSocket => ({
  socket: null,
  pingClient: 5000,
  pingDevice: 0,
});

export const useWebSocket = defineStore('webSocket', {
  state: initialState,
  actions: {
    init() {
      this.onSend('INFO');
      // this.onSend('DEVICE');
      event.emit('init');
    },
    onopen() {
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      this.init();
      event.emit('connected', true);
    },
    onmessage(message: MessageEvent) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const data = struct.get(message.data);
        event.emit('messages', data);
        if (data) {
          const { object, key } = data;
          if (key !== 'PING') log(object, key);
          const store = useWebSocketStore();

          const isStoreMethod = (key: string): key is keyof typeof store => key in store && typeof store[key as keyof typeof store] === 'function';

          if (isStoreMethod(`SET_${key}`)) {
            const methodName = `SET_${key}` as keyof typeof store;
            const method = store[methodName] as Function;
            method.call(store, object);
          } else store.SET_MAIN(data);
        }
      }
    },
    onclose(data: any) {
      event.emit('connected', false);
      log(data);
    },
    onerror(data: any) {
      event.emit('connected', false);
      log(data);
    },
    onSend(comm: string, data?: any) {
      log(comm, data);
      if (this.socket && this.socket.readyState === WebSocket.OPEN && this.isConnect) {
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
