import Vue from 'vue';
import { struct } from '@/assets/js/struct/';
export default {
  namespaced: true,
  state: {
    isConnected: false,
    message: '',
    reconnectError: false,
    struct: struct
  },
  mutations: {
    SOCKET_ONOPEN (state) {
      state.isConnected = true;
    },
    SOCKET_ONCLOSE (state) {
      state.isConnected = false;
    },
    SOCKET_ONERROR (state, event) {
      console.error(state, event);
    },
    SOCKET_ONMESSAGE (state, message) {
      if (message.data instanceof ArrayBuffer) {
        // const obj = state.struct.get(message.data)
        // console.log()
        // dispatch('sendMessage', buffer)
      }
      state.message = message;
    },
    SOCKET_RECONNECT (state, count) {
      console.log(state, count);
    },
    SOCKET_RECONNECT_ERROR (state) {
      state.reconnectError = true;
    },
  },
  actions: {
    onMessage ({ state, commit }, message) {
      // console.log(message.data)
      if (message.data instanceof ArrayBuffer) {
        const obj = state.struct.get(message.data)
        console.log(obj)
        commit(`app/SET_${obj['key']}`, JSON.parse(JSON.stringify(obj)), { root: true })
      }
    },
    sendMessage ({ state: { isConnected } }, message) {
      if (isConnected) {
        Vue.prototype.$socket.send(message);
      } else {
        console.warn(`isConnected: ${isConnected}`)
      }
      console.log(message)
    },
    onSend ({ dispatch, state: { struct } }, { comm, data }) {
      console.log(comm, data)
      const buffer = struct.set(comm, data)
      if (buffer) {
        dispatch('sendMessage', buffer)
      }
    },
  },
};
