import { createApp } from 'vue';
import { createPinia } from 'pinia';

import { struct } from '@/assets/js/';

import App from '@/App.vue';
import router from '@/router/index.ts';

import vuesp from 'vuesp-components';
import { useFetch } from 'vuesp-components/helpers';
import { i18n } from 'vuesp-components/plugins';

import 'vuesp-components/dist/style.css';

import '@/assets/tailwind.css';

const pinia = createPinia();
const app = createApp(App);

(async () => {
  const resStruct = await useFetch.$get(`/struct.json`);
  const { locales, ...resDefault } = await useFetch.$get(`/default.json`);
  struct.init(resStruct);
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
