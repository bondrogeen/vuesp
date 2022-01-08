import Vue from 'vue';
import Vuex from 'vuex';
Vue.use(Vuex);

import app from './modules/app';
import socket from './modules/socket';

export default new Vuex.Store({
  namespaced: true,
  modules: {
    app,
    socket
  },
});
