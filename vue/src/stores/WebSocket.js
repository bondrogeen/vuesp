import { defineStore } from 'pinia';
import struct from '@/assets/js/struct/';
import { useWebSocketStore } from './WebSocketStore';

export const useWebSocket = defineStore('websocket', {
  state: () => ({
    socket: null,
    pingClient: 5000,
    pingDevice: 0,
    debug: process.env.NODE_ENV === 'development',
  }),
  actions: {
    onopen(data) {
      console.log(data);
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      this.onSend('INFO');
    },
    onmessage(message) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const obj = struct.get(message.data);
        if (this.debug && obj.key !== 'PING') console.log(obj);
        if (obj) {
          const store = useWebSocketStore();
          const nameAction = `SET_${obj['key']}`;
          if (store?.[nameAction]) {
            store[nameAction](JSON.parse(JSON.stringify(obj)));
          } else {
            store.SET_UNKNOWN(JSON.parse(JSON.stringify(obj)));
          }
        }
      }
    },
    onclose(data) {
      console.log(data);
    },
    onerror(data) {
      console.log(data);
    },
    onSend(comm, data) {
      if (this.debug) console.log(comm, data);
      if (this?.socket?.send && this.isConnect) {
        const buffer = struct.set(comm, data);
        if (buffer) this.socket.send(buffer);
      }
    },
    onPing() {
      this.pingClient = Date.now();
    },
  },
  getters: {
    isConnect(state) {
      return state.pingClient - state.pingDevice < 3000;
    },
  },
});