<template>
  <div class="h-[100dvh] min-h-full flex flex-col">
    <app-overlay v-if="!isConnect">
      <div class="text-2xl font-bold mb-4">Disconnected</div>

      <div class="flex justify-center">
        <v-loader class="text-primary"></v-loader>
      </div>
    </app-overlay>

    <div class="flex h-screen overflow-hidden">
      <app-aside v-if="!isIframe" :isSidebar="isSidebar" @sidebar="onSidebar">
        <h3 class="mb-4 text-xs h-5 uppercase flex items-center gap-2 justify-between" :class="isSidebar ? 'lg:hidden' : ''">
          <span v-if="nameDevice" class="text-gray-400 text-nowrap">{{ nameDevice }}</span>
        </h3>

        <app-nav :menu="menu" :isSidebar="isSidebar" :fullPath="fullPath" @sidebar="onSidebar" />

        <div class="flex-auto"></div>

        <BlockStatus v-bind="main.info" :pkg="pkg" class="mb-4 w-full rounded-2xl bg-gray-100 px-4 py-4 text-center dark:bg-white/[0.03]" :class="isSidebar ? 'lg:hidden' : ''" />
      </app-aside>

      <div class="relative flex flex-1 flex-col overflow-y-auto overflow-x-hidden scrollbar">
        <app-header v-if="!isIframe" :change-theme="changeTheme" :notifications="notifications" @sidebar="isSidebar = !isSidebar" @notif="isNotif = !isNotif"></app-header>

        <main :class="isIframe ? 'no-scrollbar' : 'px-4 py-6 sm:px-6 lg:px-8 flex-auto'">
          <div :class="isIframe ? '' : 'container mx-auto'">
            <router-view />
          </div>
        </main>

        <app-progress v-bind="progress" :timeout="2000" class="fixed right-4 md:right-10 lg:right-20 top-20 z-20" @close="webSocketStore.SET_PROGRESS" />

        <app-notification
          :isNotif="isNotif"
          :notifications="notifications"
          @close="isNotif = false"
          @remove="webSocketStore.REMOVE_NOTIFICATION"
          @read="webSocketStore.READ_NOTIFICATION"
          @read-all="webSocketStore.READ_ALL_NOTIFICATION"
        />
      </div>
    </div>

    <app-dialog v-if="dialog.value" v-bind="dialog" @close="dialog = {}" />

    <app-dialog v-if="dialogInfo" size="md" title="Information" @close="dialogInfo = false">
      <BlockInfo v-bind="main.info" :pkg="pkg" class="w-full" />
    </app-dialog>
  </div>
</template>

<script setup lang="ts">
import { ref, computed } from 'vue';
import { useRoute } from 'vue-router';

import { useWebSocketStore } from '@/stores/WebSocketStore';

import { BlockStatus, BlockInfo } from 'vuesp-components';

import { useFrame } from '@/composables/useFrame';
import { useSocket } from '@/composables/useSocket';
import { useConnection } from '@/composables/useConnection';

const { isIframe } = useFrame();
const { isConnect } = useSocket();
const { pkg, menu, dialog, dialogInfo, main, notifications, progress, changeTheme } = useConnection();

const webSocketStore = useWebSocketStore();

const isSidebar = ref(false);
const isNotif = ref(false);

const route = useRoute();

const fullPath = computed(() => route.fullPath);
const nameDevice = computed(() => main.value?.info?.name || '');
const onSidebar = (value: boolean) => (isSidebar.value = value);
</script>
