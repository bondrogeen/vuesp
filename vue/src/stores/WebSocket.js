import { defineStore } from 'pinia';
import Struct from '../assets/js/struct/index';
import { useWebSocketStore } from './WebSocketStore';

const struct = new Struct();

export const useWebSocket = defineStore('websocket', {
  state: () => ({
    socket: null,
    pingClient: 5000,
    pingDevice: 0,
    debug: process.env.NODE_ENV === 'development',
  }),
  actions: {
    onInit() {
      console.log(struct)
      this.onSend('INFO');
    },
    onopen(data) {
      console.log(data);
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      struct.onInit = this.onInit;
      this.onSend('INIT');
    },
    onmessage(message) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const obj = struct.get(message.data);
        if (obj) {
          if (this.debug && obj?.key !== 'PING') console.log(obj);
          const store = useWebSocketStore();
          const action = store?.[`SET_${obj['key']}`] || store.SET_UNKNOWN;
          action(obj);
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
    isConnect: state => state.pingClient - state.pingDevice < 3000,
  },
});
