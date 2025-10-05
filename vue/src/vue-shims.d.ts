declare module '*.vue' {
  import { defineComponent } from 'vue';
  const component: ReturnType<typeof defineComponent>;
  export default component;
}

// import { ComponentCustomProperties } from 'vue';

// declare module '@vue/runtime-core' {
//   interface ComponentCustomProperties {
//     $vuesp: {
//       name: string;
//       version: string;
//       author: string;
//       email: string;
//       homepage: string;
//       description: string;
//       repository: { type: string; url: string };
//       bugs: { url: string };
//       license: string;
//     };
//   }
// }
