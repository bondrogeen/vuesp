import { createApp } from 'vue';
import { createPinia } from 'pinia';
import { useWebSocket } from './stores/WebSocket';

import App from './App.vue';
import router from './router';

import '@/assets/tailwind.css';

import { add } from './components/global';
import directives from '@/utils/directives';

const pinia = createPinia();
const app = createApp(App);

directives.forEach(directive => app.directive(directive.name, directive));
add(app);

(async () => {
  app.use(pinia);
  const store = useWebSocket();
  await store.onStruct();
  app.use(router);
  app.mount('body');
})();
