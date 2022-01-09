import Vue from 'vue';
import App from './App.vue';
import router from './router';
import store from './store';
const host = process.env.NODE_ENV === 'production' ? window.location.host : process.env.PROXY

import VueNativeSock from './assets/js/nativeWebSocket/Main';
Vue.use(VueNativeSock, `ws://${host}/ws`, {
  store: store,
  reconnection: true,
  reconnectionAttempts: 5,
  reconnectionDelay: 3000,
});

import AtComponents from '@/at-ui/src'
import '@/at-ui/scss/index.scss'

Vue.use(AtComponents)

import '@/assets/scss/main.scss'

Vue.config.productionTip = false;

new Vue({
  router,
  store,
  render: h => h(App),
}).$mount('#app');
