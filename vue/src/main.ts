import { createApp } from 'vue';
import { createPinia } from 'pinia';

import { useWebSocket } from '@/stores/WebSocket';

import App from '@/App.vue';
import router from '@/router/index.ts';

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import VuespComponents from 'vuesp-components';
import 'vuesp-components/dist/vuesp-components.css';

import '@/assets/tailwind.css';

// import { add } from "./components/global";
import directives from '@/utils/directives/index.ts';

const pinia = createPinia();
const app = createApp(App);

directives.forEach(({ name, directive }) => app.directive(name, directive));
// add(app);

(async () => {
  app.use(pinia);
  app.use(VuespComponents);
  const store = useWebSocket();
  await store.onStruct();
  app.use(router);
  app.mount('#appVuesp');
})();
