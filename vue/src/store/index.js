import Vue from 'vue';
import Vuex from 'vuex';
import { struct } from '../assets/js/struct/';
Vue.use(Vuex);

import app from './modules/app';
import settings from './modules/settings';

export default new Vuex.Store({
  modules: {
    app,
    settings
  },
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
        console.log(state.struct.get(message.data))
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
    sendMessage (context, message) {
      Vue.prototype.$socket.send(message);
    },
    sendStruct ({ dispatch, state: { struct } }, { comm, data }) {
      const buffer = struct.set(comm, data)
      if (buffer) {
        dispatch('sendMessage', buffer)
      }
    },
  },
});
