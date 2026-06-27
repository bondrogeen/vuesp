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
  try {
    const resStruct = await useFetch(`/struct.json`).get().json();
    const res = await useFetch(`/default.json`).get().json();
    
    if (!resStruct.data.value || !res.data.value) {
      throw new Error('Failed to load configuration from device');
    }
    
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
  } catch (error) {
    console.error('Failed to initialize application:', error);
    const errorEl = document.getElementById('appVuesp');
    if (errorEl) {
      errorEl.innerHTML = `
        <div style="padding: 2rem; text-align: center;">
          <h1>Failed to Load Application</h1>
          <p>${error instanceof Error ? error.message : 'Unknown error occurred'}</p>
          <p style="color: #666; margin-top: 1rem;">Please check that the device is accessible and try refreshing the page.</p>
          <button onclick="location.reload()" style="padding: 0.5rem 1rem; margin-top: 1rem; cursor: pointer;">
            Reload Page
          </button>
        </div>
      `;
    }
  }
})();
