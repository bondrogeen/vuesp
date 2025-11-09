import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { useFetch } from '@vueuse/core';

import { struct } from '@/assets/js/';

import App from '@/App.vue';
import router from '@/router/index.ts';

import vuesp from 'vuesp-components';
import { i18n } from 'vuesp-components/plugins';

import 'vuesp-components/dist/style.css';

import '@/assets/tailwind.css';

const pinia = createPinia();
const app = createApp(App);

(async () => {
  const resStruct = await useFetch(`/struct.json`).get().json();
  const res = await useFetch(`/default.json`).get().json();
  const { locales, ...resDefault } = res.data.value;
  struct.init(resStruct.data.value);
  pinia.use(({ store }) => {
    if (resDefault && store.$id === 'app') {
      store.$patch(resDefault);
      // @ts-ignore: Unreachable code error
      store.$patch({ pkg: __APP__ });
    }
  });
  app.use(i18n, { locales });
  app.use(pinia);
  app.use(vuesp);
  app.use(router);
  app.mount('#appVuesp');
})();
