import { createApp } from 'vue';
import { createPinia } from 'pinia';

import { struct } from '@/assets/js/';

import App from '@/App.vue';
import router from '@/router/index.ts';

import vuesp from 'vuesp-components';

import 'vuesp-components/dist/style.css';

import '@/assets/tailwind.css';

import { PKGKey } from '@/utils/simbol';

const pinia = createPinia();
const app = createApp(App);

(async () => {
  // @ts-ignore: Unreachable code error
  app.provide(PKGKey, __APP__);
  const res = await (await fetch(`/struct.json`, { method: 'GET' })).json();
  console.log(res);

  struct.init(res);
  app.use(pinia);
  app.use(vuesp);
  app.use(router);
  app.mount('#appVuesp');
})();
