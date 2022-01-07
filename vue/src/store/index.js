import Vue from 'vue';
import Vuex from 'vuex';
Vue.use(Vuex);

import app from './modules/app';

export default new Vuex.Store({
  modules: {
    app,
  },
  state: {
    isConnected: false,
    message: '',
    reconnectError: false,
  },
  mutations: {
    SOCKET_ONOPEN(state) {
      // Vue.prototype.$socket = event.currentTarget;
      state.isConnected = true;
    },
    SOCKET_ONCLOSE(state) {
      state.isConnected = false;
    },
    SOCKET_ONERROR(state, event) {
      console.error(state, event);
    },
    // default handler called for all methods
    SOCKET_ONMESSAGE(state, message) {
      state.message = message;
    },
    // mutations for reconnect methods
    SOCKET_RECONNECT(state, count) {
      console.log(state, count);
    },
    SOCKET_RECONNECT_ERROR(state) {
      state.reconnectError = true;
    },
  },
  actions: {
    sendMessage: function (context, message) {
      console.log(context);
      Vue.prototype.$socket.send(message);
    },
  },
});
