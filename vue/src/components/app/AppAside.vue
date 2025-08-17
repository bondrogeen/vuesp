<template>
  <aside
    :class="sidebarToggle ? 'translate-x-0 lg:w-[90px]' : '-translate-x-full'"
    class="sidebar fixed top-0 left-0 z-10 flex h-screen w-[290px] flex-col overflow-y-auto border-r border-gray-200 px-5 transition-all duration-300 lg:static lg:translate-x-0 dark:border-gray-800 bg-white dark:bg-gray-900 -translate-x-full"
  >
    <div :class="sidebarToggle ? 'justify-center' : 'justify-between'" class="sidebar-header flex items-center gap-2 pt-6 pb-6 justify-between">
      <router-link to="/">
        <icon-logo-mini v-if="sidebarToggle" class="h-[30px]" />

        <icon-logo v-else class="h-[30px]" />
      </router-link>
    </div>

    <div class="no-scrollbar flex flex-col overflow-y-auto duration-300 ease-linear h-full">
      <nav>
        <div>
          <h3 class="mb-4 text-xs h-5 text-gray-400 uppercase">
            <span class="menu-group-title" :class="sidebarToggle ? 'lg:hidden' : ''">{{ info.name }}</span>

            <icon-dots :class="sidebarToggle ? 'lg:block hidden' : 'hidden'" class="menu-group-icon mx-auto fill-current hidden" />
          </h3>

          <ul class="mb-6 flex flex-col gap-4">
            <li v-for="{ name, path, icon, children } of menu" :key="name">
              <component
                :is="!children ? 'router-link' : 'span'"
                :to="path"
                class="flex gap-2 items-center px-3 py-2 rounded-sm"
                :class="isActive(path) ? 'bg-blue-500/10' : 'hover:bg-gray-500/10'"
                @click.prevent="onSelect(name)"
              >
                <component :is="icon" class="h-5 w-5 flex-[0_0_24px]" />

                <span class="flex-1 first-letter:uppercase" :class="sidebarToggle ? 'lg:hidden' : ''">{{ name }}</span>

                <icon-chevron v-if="children" :class="[sidebarToggle ? 'lg:hidden' : '']" />
              </component>

              <div v-if="children" class="translate transform overflow-hidden" :class="selected === name ? 'block' : 'hidden'">
                <ul :class="sidebarToggle ? 'lg:hidden' : 'flex'" class="mt-2 flex flex-col gap-1 pl-9">
                  <li v-for="item of children" :key="item.name">
                    <router-link :to="item?.path || ''" class="block px-2 py-2 rounded-sm first-letter:uppercase" :class="isActive(item.path) ? 'bg-blue-500/10' : 'hover:bg-gray-500/10'">
                      {{ item.name }}
                    </router-link>
                  </li>
                </ul>
              </div>
            </li>
          </ul>
        </div>
      </nav>

      <div class="flex-auto"></div>

      <ServiceInfo v-bind="info" class="mb-4 w-full rounded-2xl bg-gray-50 px-4 py-4 text-center dark:bg-white/[0.03]" :class="sidebarToggle ? 'lg:hidden' : ''">
        <v-button href="https://github.com/bondrogeen/vuesp" class="w-full" target="_blank" color="blue">Github</v-button>
      </ServiceInfo>
    </div>
  </aside>
</template>

<script setup lang="ts">
import { onMounted, ref } from 'vue';
import { useRoute } from 'vue-router';

import type { IMenuItem, IMenuChild, IStateInfo } from '@/types/types.ts';

import ServiceInfo from '@/components/pages/service/ServiceInfo.vue';

interface Props {
  sidebarToggle?: boolean;
  menu?: IMenuItem[];
  info?: IStateInfo;
}

const { info = {}, sidebarToggle = false, menu = [] } = defineProps<Props>();

// const emit = defineEmits<{
//   (e: 'sidebar', value: boolean): void;
// }>();

const route = useRoute();

const onSelect = (name: string = '') => (selected.value = selected.value === name ? '' : name);

const isActive = (path: string = '') => route.fullPath === path;

const selected = ref('');

// const onHover = (value: boolean) => {
//   if (sidebarToggle) {
//     emit('sidebar', value);
//   }
// };

onMounted(() => {
  setTimeout(() => {
    menu.forEach((item: IMenuItem) => {
      const el = (item?.children || []).find((i: IMenuChild) => isActive(i?.path));
      if (el) selected.value = item.name;
    });
  }, 300);
});
</script>
