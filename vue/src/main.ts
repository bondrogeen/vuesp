import { createApp } from 'vue';
import { createPinia } from 'pinia';

import { struct } from '@/utils/struct.ts';

import App from '@/App.vue';
import router from '@/router/index.ts';

import VuespComponents from 'vuesp-components';

import 'vuesp-components/dist/style.css';

import '@/assets/tailwind.css';

import { VuespKey } from '@/utils/types/simbol';

const pinia = createPinia();
const app = createApp(App);

(async () => {
  // @ts-ignore: Unreachable code error
  app.provide(VuespKey, __APP__);
  const res = await (await fetch(`/struct.json`, { method: 'GET' })).json();
  struct.init(res);
  app.use(pinia);
  app.use(VuespComponents);
  app.use(router);
  app.mount('#appVuesp');
})();
