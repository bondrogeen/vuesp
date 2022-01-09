import Vue from 'vue';
import App from './App.vue';
import router from './router';
import store from './store';
const host = process.env.NODE_ENV === 'production' ? window.location.host : process.env.PROXY

import VueNativeSock from './assets/js/nativeWebSocket/Main';
Vue.use(VueNativeSock, `ws://${host}/ws`, {
  store: store,
  reconnection: true,
  reconnectionAttempts: 5, // (Number) number of reconnection attempts before giving up (Infinity),
  reconnectionDelay: 3000, // (Number) how long to initially wait before attempting a new (1000)
});


// import AtComponents from 'at-ui'
// import '../src/at-ui/scss/index.scss'    // Import CSS
import AtComponents from '@/at-ui/src'
import '@/at-ui/scss/index.scss'
import '@/assets/scss/main.scss'
// import 'at-ui-style/src/index.scss'      // Or import the unbuilt version of SCSS

Vue.use(AtComponents)

Vue.config.productionTip = false;

new Vue({
  router,
  store,
  render: h => h(App),
}).$mount('#app');
