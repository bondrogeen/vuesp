import { createApp } from 'vue';
import { createPinia } from 'pinia';
import App from './App.vue';
import router from './router';

import '@/assets/scss/index.scss';

import { add } from './components/global';
import directives from '@/utils/directives';

const pinia = createPinia();
const app = createApp(App);

directives.forEach(directive => app.directive(directive.name, directive));

add(app);
app.use(pinia);
app.use(router);
app.mount('body');
