<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <app-overlay v-if="!isConnect">
      <div class="text-2xl font-bold mb-4">{{ $t('disconn') }}</div>

      <div class="flex justify-center">
        <v-loader class="text-primary"></v-loader>
      </div>
    </app-overlay>

    <div class="flex h-screen overflow-hidden">
      <app-aside v-if="!isIframe" :isSidebar="isSidebar" @sidebar="onSidebar">
        <h3 class="mb-4 text-xs h-5 uppercase flex items-center gap-2 justify-between" :class="isSidebar ? 'lg:hidden' : ''">
          <span v-if="nameDevice" class="text-gray-400 text-nowrap">{{ nameDevice }}</span>
        </h3>

        <app-nav :menu="getMenu" :isSidebar="isSidebar" :fullPath="fullPath" @sidebar="onSidebar" />

        <div class="flex-auto"></div>

        <BlockStatus v-slot="{ repo, home }" v-bind="info" :pkg="pkg" class="mb-4 w-full rounded-2xl bg-gray-100 px-4 py-4 text-center dark:bg-white/[0.03]" :class="isSidebar ? 'lg:hidden' : ''">
          <div class="flex gap-2 mt-4">
            <v-button :href="repo" target="_blank" class="w-full text-white" color="blue">Github</v-button>

            <v-button v-if="home" :href="home" target="_blank" class="w-full" color="blue" outline>{{ $t('homepage') }}</v-button>
          </div>
        </BlockStatus>
      </app-aside>

      <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden scrollbar">
        <app-header v-if="!isIframe" :change-theme="changeTheme" :notifications="notifications" @sidebar="isSidebar = !isSidebar" @notif="isSidebarRight = !isSidebarRight"></app-header>

        <main :class="isIframe ? 'no-scrollbar' : 'px-4 py-6 sm:px-6 lg:px-8 flex-auto'">
          <div :class="isIframe ? '' : 'container mx-auto'">
            <router-view />
          </div>
        </main>

        <app-progress v-slot="{ isDone }" v-bind="progress" :timeout="2000" class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" @close="webSocketStore.SET_PROGRESS">
          <div class="flex-auto">{{ $t('prog') }}</div>

          <span v-if="isDone" class="text-green-600">{{ $t('done') }}</span>
        </app-progress>

        <app-notification :value="isSidebarRight" :notifications="notifications" @close="isSidebarRight = false" @remove="webSocketStore.REMOVE_NOTIFICATION" @read="webSocketStore.READ_NOTIFICATION">
          <template #header="{ isNew }">
            <div class="flex items-center justify-between py-4 sticky top-0 z-20 bg-gray-50 dark:bg-gray-900 px-2">
              <h5>{{ $t('noti') }}</h5>

              <VButton size="small" color="gray" class="text-sm" :disabled="!isNew" @click="webSocketStore.READ_ALL_NOTIFICATION">{{ $t('mark') }}</VButton>
            </div>
          </template>
          <template #empty>
            <p class="text-center text-gray-200/50 py-10">{{ $t('empty') }}</p>
          </template>
        </app-notification>
      </div>
    </div>

    <app-dialog v-if="dialog.value" v-bind="dialog" @close="dialog = {}" />

    <app-dialog v-if="dialogInfo" size="md" title="Information" @close="dialogInfo = false">
      <BlockInfo v-bind="main.info" :pkg="pkg" class="w-full">
        <template #links="{ links }">
          {{ $t('links') }}:
          <div class="flex flex-wrap gap-2 text-gray-700 dark:text-gray-400">
            <a v-for="link of links" :key="link.name" :href="link.href" target="_blank" class="me-4">{{ $t(link.name) }}</a>
          </div>
        </template>
      </BlockInfo>
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import { useRoute } from 'vue-router';

import { localGet } from 'vuesp-components/helpers';

import { useWebSocketStore } from '@/stores/WebSocketStore';

import { useFrame } from '@/composables/useFrame';
import { useSocket } from '@/composables/useSocket';
import { useStore } from '@/composables/useStore';
import { useLocale } from '@/composables/useLocale';

import { BlockInfo } from 'vuesp-components';
import BlockStatus from '@/components/block/BlockStatus.vue';

const { $t, setLocale } = useLocale();

setLocale(localGet('locale') || navigator?.language);

const { isIframe } = useFrame();
const { isConnect } = useSocket();
const { pkg, menu, dialog, dialogInfo, main, notifications, progress, changeTheme } = useStore();

const webSocketStore = useWebSocketStore();

const isSidebar = ref(false);
const isSidebarRight = ref(false);

const route = useRoute();

const getMenu = computed(() => menu.value.map((i) => ({ ...i, name: $t(i.name) })));
const info = computed(() => main.value.info);
const nameDevice = computed(() => info.value?.name || '');

const fullPath = computed(() => route.fullPath);
const onSidebar = (value: boolean) => (isSidebar.value = value);
</script>
