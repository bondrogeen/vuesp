import { createApp } from 'vue';
import { createPinia } from 'pinia';
// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import { useWebSocket } from '@/stores/WebSocket';

// eslint-disable-next-line @typescript-eslint/ban-ts-comment
// @ts-ignore
import App from '@/App.vue';
import router from '@/router/index.ts';

import '@/assets/tailwind.css';

// import { add } from "./components/global";
import directives from '@/utils/directives/index.ts';

const pinia = createPinia();
const app = createApp(App);

directives.forEach(({ name, directive }) => app.directive(name, directive));
// add(app);

(async () => {
  app.use(pinia);
  const store = useWebSocket();
  await store.onStruct();
  app.use(router);
  app.mount('body');
})();
