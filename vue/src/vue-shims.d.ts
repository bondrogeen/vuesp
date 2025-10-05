declare module '*.vue' {
  import { defineComponent } from 'vue';
  const component: ReturnType<typeof defineComponent>;
  export default component;
}

import type { I18nInstance } from './i18n';

declare module '@vue/runtime-core' {
  interface ComponentCustomProperties {
    $i18n: I18nInstance;
  }
}

export {};
