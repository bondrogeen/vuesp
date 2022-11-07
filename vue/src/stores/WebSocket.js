import { defineStore } from 'pinia';
import Struct from '@/assets/js/struct';
import { useWebSocketStore } from './WebSocketStore';
import event from '@/assets/js/event';
import log from '@/utils/other/debug';


const struct = new Struct();

export const useWebSocket = defineStore('websocket', {
  state: () => ({
    socket: null,
    pingClient: 5000,
    pingDevice: 0,
  }),
  actions: {
    onInit() {
      this.onSend('INFO');
      event.emit('init');
    },
    onopen() {
      this.pingDevice = Date.now();
      this.pingClient = Date.now();
      struct.onInit = this.onInit;
      this.onSend('INIT');
      event.emit('connected', true);
    },
    onmessage(message) {
      this.pingDevice = Date.now();
      if (message.data instanceof ArrayBuffer) {
        const data = struct.get(message.data);
        if (data) {
          const { object, key } = data;
          if (key !== 'PING') log(object, key);
          const store = useWebSocketStore();
          (store?.[`SET_${key}`] || store.SET_UNKNOWN)(object);
        }
      }
    },
    onclose(data) {
      event.emit('connected', false);
      log(data);
    },
    onerror(data) {
      event.emit('connected', false);
      log(data);
    },
    onSend(comm, data) {
      log(comm, data);
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
